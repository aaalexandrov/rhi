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

	auto gridTex = eng::Sys::Get()->LoadTexture("data/grid2.png", true);

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
	world->CreateCamera();

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
	eng::Sys::Get()->_scene = eng::Sys::Get()->_world->CreateScene();
	ASSERT(eng::Sys::Get()->_scene->_camera);

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
	std::chrono::time_point now = startTime;
	uint64_t frame = 0;
	for (; running; ) {
		eng::Sys::Get()->_ui->HandleInput();
		eng::Sys::Get()->_ui->UpdateWindows();

		std::chrono::time_point newTime = std::chrono::high_resolution_clock::now();
		std::chrono::duration<utl::UpdateQueue::Time> deltaSec = newTime - now;
		eng::Sys::Get()->UpdateTime(deltaSec.count());
		now = newTime;

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