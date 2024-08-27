#include <iostream>
#include <filesystem>

#include "eng/sys.h"
#include "eng/world.h"
#include "eng/object.h"
#include "eng/component.h"
#include "eng/render/scene.h"
#include "eng/ui/properties.h"

#include "rhi/pass.h"
#include "rhi/pipeline.h"

#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"
#include "imgui.h"
#include "stb_image.h"

struct PropTest {
	float _flt = glm::pi<float>();
	int32_t _i32 = 42;
	std::string _name = "Prop Test name";
	bool _check = false;
	glm::vec3 _pos = glm::vec3(1, 2, 3);
	uint16_t _vals[8];
	PropTest *_next = nullptr;
};

static auto s_regTypes = utl::TypeInfo::AddInitializer("rhi_test", [] {
	utl::TypeInfo::Register<PropTest>().Name("eng::PropTest")
		.Member("_flt", &PropTest::_flt)
		.Member("_i32", &PropTest::_i32)
		.Member("_name", &PropTest::_name)
		.Member("_check", &PropTest::_check)
		.Member("_pos", &PropTest::_pos)
		.Member("_vals", &PropTest::_vals)
		.Member("_next", &PropTest::_next)
		;
});


std::shared_ptr<rhi::Texture> LoadTexture(std::string path, bool genMips)
{
	auto rhi = eng::Sys::Get()->_rhi.get();
	int x, y, ch;
	uint8_t *img = stbi_load(path.c_str(), &x, &y, &ch, 0);

	if (!img)
		return nullptr;

	rhi::Format fmt;
	switch (ch) {
		case 1:
			fmt = rhi::Format::R8;
			break;
		case 2:
			fmt = rhi::Format::R8G8;
			break;
		case 4:
			fmt = rhi::Format::R8G8B8A8;
			break;
		default:
			return nullptr;
	}
	glm::vec4 dims{ x, y, 0, 0 };
	rhi::ResourceDescriptor texDesc{
		._usage{.srv = 1, .copySrc = genMips, .copyDst = 1},
		._format{fmt},
		._dimensions{dims},
		._mipLevels{rhi::GetMaxMipLevels(dims)},
	};
	std::shared_ptr<rhi::Texture> tex = rhi->New<rhi::Texture>(path, texDesc);

	std::shared_ptr<rhi::Buffer> staging = rhi->New<rhi::Buffer>("Staging " + path, rhi::ResourceDescriptor{
		._usage{.copySrc = 1, .cpuAccess = 1},
		._dimensions{x * y * ch, 0, 0, 0},
		});
	std::span<uint8_t> pixMem = staging->Map();
	memcpy(pixMem.data(), img, pixMem.size());
	staging->Unmap();

	stbi_image_free(img);

	std::vector<std::shared_ptr<rhi::Pass>> passes;
	auto copyStaging = rhi->Create<rhi::CopyPass>();
	copyStaging->Copy(rhi::CopyPass::CopyData{ ._src{staging}, ._dst{tex, rhi::ResourceView{._mipRange{0, 0}}} });
	passes.push_back(copyStaging);

	if (genMips) {
		auto mipGen = rhi->Create<rhi::CopyPass>();
		mipGen->CopyTopToLowerMips(tex);
		passes.push_back(mipGen);
	}

	auto sub = rhi->Submit(std::move(passes), "Upload " + path);
	sub->Prepare();
	sub->Execute();
	sub->WaitUntilFinished();

	return tex;
}

eng::Model InitTriModel(std::span<rhi::RenderTargetData> renderTargets)
{
	auto rhi = eng::Sys::Get()->_rhi.get();

	auto solidVert = rhi->GetShader("data/solid.vert", rhi::ShaderKind::Vertex);
	auto solidFrag = rhi->GetShader("data/solid.frag", rhi::ShaderKind::Fragment);

	auto solidPass = rhi->New<rhi::GraphicsPass>("solidPass", renderTargets);

	rhi::PipelineData solidData{
		._shaders = {{ solidVert, solidFrag }},
		._vertexInputs = { rhi::VertexInputData{._layout = solidVert->GetParam(rhi::ShaderParam::Kind::VertexLayout, 0)->_ownTypes[0] }},
	};
	auto solidPipe = rhi->GetPipeline(solidData, solidPass.get());

	auto *vertLayout = solidPipe->_pipelineData.GetShader(rhi::ShaderKind::Vertex)->GetParam(rhi::ShaderParam::VertexLayout);
	auto triBuf = rhi->New<rhi::Buffer>("triangle", rhi::ResourceDescriptor{
		._usage = rhi::ResourceUsage{.vb = 1, .cpuAccess = 1},
		._dimensions = glm::ivec4(vertLayout->_type->_size * 3),
		});

	{
		auto triMapped = triBuf->Map();
		utl::AnyRef vert{ vertLayout->_type, triMapped.data() };
		*vert.GetMember("pos").Get<glm::vec3>() = glm::vec3(0.25, 0.25, 0);
		*vert.GetMember("tc").Get<glm::vec2>() = glm::vec2(0.0, 0.0);
		*vert.GetMember("color").Get<glm::vec4>() = glm::vec4(1, 0, 0, 1);

		vert._instance = triMapped.data() + vert._type->_size * 1;
		*vert.GetMember("pos").Get<glm::vec3>() = glm::vec3(0.25, 0.75, 0);
		*vert.GetMember("tc").Get<glm::vec2>() = glm::vec2(0.0, 1.0);
		*vert.GetMember("color").Get<glm::vec4>() = glm::vec4(0, 1, 0, 1);

		vert._instance = triMapped.data() + vert._type->_size * 2;
		*vert.GetMember("pos").Get<glm::vec3>() = glm::vec3(0.75, 0.75, 0);
		*vert.GetMember("tc").Get<glm::vec2>() = glm::vec2(1.0, 1.0);
		*vert.GetMember("color").Get<glm::vec4>() = glm::vec4(0, 0, 1, 1);
		triBuf->Unmap();
	}

	auto sampler = rhi->New<rhi::Sampler>("samp", rhi::SamplerDescriptor{});

	auto gridTex = LoadTexture("data/grid2.png", true);

	auto mesh = std::make_shared<eng::Mesh>();
	mesh->_name = "Triangle";
	mesh->_vertices = triBuf;
	mesh->_vertexInputs = solidPipe->_pipelineData._vertexInputs;
	mesh->_numVertices = triBuf->_descriptor._dimensions[0] / vertLayout->_type->_size;
	mesh->_indexRange = utl::IntervalU(0, 2);
	mesh->_bound = mesh->GetVertexBound("pos");

	auto mat = std::make_shared<eng::Material>();
	mat->_name = "Solid";
	mat->_shaders = { solidVert, solidFrag };
	mat->_params["samp"] = sampler;
	mat->_params["tex"] = gridTex;

	eng::Model model;
	model._mesh = mesh;
	model._material = mat;
	model._pipeline = solidPipe;

	return model;
}

bool InitWorld(rhi::Swapchain *swapchain)
{
	eng::Sys::Get()->_world = std::make_unique<eng::World>();
	eng::World *world = eng::Sys::Get()->_world.get();
	world->Init("TestWorld", utl::BoxF(glm::vec3(-1024), glm::vec3(1024)), glm::vec3(4));

	{
		auto tri = std::make_shared<eng::Object>();
		tri->_name = "Triangle";
		auto triRender = tri->AddComponent<eng::RenderingCmp>();
		rhi::RenderTargetData rt{ swapchain->_images[0] };
		auto triModel = InitTriModel(std::span(&rt, 1));
		triRender->_models.push_back(std::move(triModel));
		triRender->UpdateObjectBoundFromModels();
		tri->SetWorld(world);
	}

	{
		auto camera = std::make_shared<eng::Object>();
		camera->_name = "Camera";
		camera->AddComponent<eng::CameraCmp>();
		camera->SetTransform(utl::Transform3F(glm::vec3(0, 0, 2), glm::angleAxis(0*glm::pi<float>(), glm::vec3(0, 1, 0)), 1.0f));
		camera->SetWorld(world);
	}

	return true;
}

bool InitScene(rhi::Swapchain *swapchain)
{
	eng::World *world = eng::Sys::Get()->_world.get();
	eng::CameraCmp *camera = nullptr;
	world->EnumObjects([&](std::shared_ptr<eng::Object> &obj) {
		camera = obj->GetComponent<eng::CameraCmp>();
		return camera ? utl::Enum::Stop : utl::Enum::Continue;
	});
	if (!camera)
		return false;

	std::array<rhi::RenderTargetData, 1> renderTargets{
		{
			std::shared_ptr<rhi::Texture>(),
			glm::vec4(0, 0, 1, 1),
		},
	};

	eng::Sys::Get()->_scene = std::make_unique<eng::Scene>(world, camera, renderTargets);
	eng::Scene *scene = eng::Sys::Get()->_scene.get();

	return true;
}

int main()
{
	std::cout << "Starting in " << std::filesystem::current_path() << std::endl;

	utl::TypeInfo::Init();
	utl::OnDestroy typesDone(utl::TypeInfo::Done);

	eng::Sys::InitInstance();
	utl::OnDestroy sysDone(eng::Sys::DoneInstance);

	auto window = eng::Sys::Get()->_ui->NewWindow(eng::WindowDescriptor{
		._name = "Eng Test",
		._swapchainDesc{
			._usage{.rt = 1, .copySrc = 1, .copyDst = 1},
			._format = rhi::Format::B8G8R8A8_srgb,
			._presentMode = rhi::PresentMode::Fifo,
		},
	});

	eng::Sys::Get()->InitRhi(window);

	InitWorld(window->_swapchain.get());
	InitScene(window->_swapchain.get());

	rhi::Rhi *rhi = eng::Sys::Get()->_rhi.get();

	bool running = true;
	window->_imguiCtx->_fnInput = [&](eng::Window *win, SDL_Event const &event) {
		if (event.type == SDL_WINDOWEVENT) {
			if (event.window.event == SDL_WINDOWEVENT_CLOSE)
				running = false;
		}

		if (event.type == SDL_KEYDOWN) {
			if (event.key.keysym.sym == SDLK_v) {
				uint32_t supportedModes = win->_swapchain->GetSupportedPresentModeMask();
				uint32_t clearMask = ~((1 << ((uint32_t)win->_swapchain->_descriptor._presentMode + 1)) - 1);
				uint32_t nextModes = supportedModes & clearMask;
				if (!nextModes)
					nextModes = supportedModes;
				auto presentMode = (rhi::PresentMode)std::countr_zero(nextModes);
				win->_swapchain->Update(presentMode);
			}
		}
	};

	std::chrono::time_point startTime = std::chrono::high_resolution_clock::now();
	std::chrono::time_point nowTime = startTime;
	uint64_t frame = 0;
	for (; running; ) {
		eng::Sys::Get()->_ui->HandleInput();
		eng::Sys::Get()->_ui->UpdateWindows();

		eng::Sys::Get()->UpdateTime();

		glm::ivec2 swapchainSize = glm::ivec2(window->_swapchain->_descriptor._dimensions);
		if (any(equal(swapchainSize, glm::ivec2(0))))
			continue;

		auto swapchainTexture = window->_swapchain->AcquireNextImage();
			
		window->_imguiCtx->LayoutUi([] {
			ImGui::Begin("Fps", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground /* | ImGuiWindowFlags_AlwaysAutoResize */);
			ImGui::SetWindowPos(ImVec2(10, 10), ImGuiCond_Once);
			ImGui::SetWindowSize(ImVec2(100, 20), ImGuiCond_Once);
			ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
			ImGui::End();

			static PropTest tst, tst1;
			tst._next = &tst1;
			tst1._next = &tst;
			eng::Sys::Get()->_ui->_keyboardFocusInUI = eng::DrawPropertiesWindow("Properties of Obj", utl::AnyRef::From(tst));

			//ImGui::ShowDemoWindow();
		});


		std::array<rhi::RenderTargetData, 1> renderTargets{
			{
				swapchainTexture,
				glm::vec4(0, 0, 1 ,1),
			},
		};

		eng::Scene *scene = eng::Sys::Get()->_scene.get();

		scene->_renderTargets.clear();
		scene->_renderTargets.insert(scene->_renderTargets.end(), renderTargets.begin(), renderTargets.end());

		eng::RenderObjectsData renderObjData = scene->RenderObjects();

		window->_imguiCtx->Render(renderObjData._renderPass.get());

		std::vector<std::shared_ptr<rhi::Pass>> passes{ renderObjData._updatePasses.begin(), renderObjData._updatePasses.end() };

		passes.push_back(renderObjData._renderPass);

		auto presentPass = rhi->Create<rhi::PresentPass>("Present");
		presentPass->SetSwapchainTexture(swapchainTexture);
		passes.push_back(presentPass);

		auto submission = rhi->Submit(std::move(passes), "Execute");
		bool res = submission->Prepare();
		ASSERT(res);
		res = submission->Execute();
		ASSERT(res);
		res = submission->WaitUntilFinished();
		ASSERT(res);

		++frame;
	}

	double runtime = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - startTime).count();
	std::cout << "Run time " << runtime << " seconds, " << frame << " frames, " << frame / runtime << " fps.\n";
	std::cout << "Bye!\n"; 

	return 0;
}