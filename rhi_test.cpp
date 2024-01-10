#include <iostream>
#include <filesystem>

#include "eng/sys.h"

#include "rhi/pass.h"
#include "rhi/pipeline.h"

#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"
#include "backends/imgui_impl_vulkan.h"
#include "backends/imgui_impl_sdl2.h"

int main()
{
	std::cout << "Starting in " << std::filesystem::current_path() << std::endl;

	utl::TypeInfo::Init();
	utl::OnDestroy typesDone(utl::TypeInfo::Done);

	eng::Sys::InitInstance();
	utl::OnDestroy sysDone(eng::Sys::DoneInstance);

	auto window = std::make_shared<eng::Window>();
	window->Init(eng::Window::Descriptor{
		._name = "Eng Test",
	});

	eng::Sys::Get()->InitRhi(window);
	rhi::SwapchainDescriptor chainDesc{
		._usage{.rt = 1, .copySrc = 1, .copyDst = 1},
		._format = rhi::Format::B8G8R8A8_srgb,
		._presentMode = rhi::PresentMode::Fifo,
	};
	window->InitRendering(chainDesc);

	auto device = eng::Sys::Get()->_rhi.get();

	auto solidVert = device->Create<rhi::Shader>();
	bool res = solidVert->Load("data/solid.vert", rhi::ShaderKind::Vertex);
	ASSERT(res);

	auto solidFrag = device->Create<rhi::Shader>();
	res = solidFrag->Load("data/solid.frag", rhi::ShaderKind::Fragment);
	ASSERT(res);

	std::array<rhi::GraphicsPass::TargetData, 1> solidRts{{	window->_swapchain->_images[0] }};
	auto solidPass = device->New<rhi::GraphicsPass>("solidPass", std::span(solidRts));

	rhi::GraphicsPipelineData solidData{
		._shaders = {{ solidVert, solidFrag }},
		._renderPass = std::move(solidPass),
		._vertexInputs = { rhi::VertexInputData{._layout = solidVert->GetParam(rhi::ShaderParam::Kind::VertexLayout, 0)->_ownTypes[0] }},
	};
	auto solidPipe = device->New<rhi::Pipeline>("", solidData);

	auto *vertLayout = solidPipe->GetShader(rhi::ShaderKind::Vertex)->GetParam(rhi::ShaderParam::VertexLayout);
	auto triBuf = device->New<rhi::Buffer>("triangle", rhi::ResourceDescriptor{
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

	auto *solidUniformLayout = solidPipe->GetShader(rhi::ShaderKind::Vertex)->GetParam(rhi::ShaderParam::UniformBuffer);
	auto solidUniform = device->New<rhi::Buffer>("solidUniform", rhi::ResourceDescriptor{
		._usage = rhi::ResourceUsage{.srv = 1, .cpuAccess = 1},
		._dimensions = glm::ivec4(solidUniformLayout->_type->_size),
	});

	{
		auto solidMapped = solidUniform->Map();
		utl::AnyRef uni{ solidUniformLayout->_type, solidMapped.data() };
		glm::mat4 mat{ glm::vec4(2, 0, 0, 0), glm::vec4(0, 2, 0, 0), glm::vec4(0, 0, 1, 0), glm::vec4(-1, -1, 0, 1) };
		*uni.GetMember("view_proj").Get<glm::mat4>() = mat;
		solidUniform->Unmap();
	}

	auto sampler = device->New<rhi::Sampler>("samp", rhi::SamplerDescriptor{});

	auto solidResSet = solidPipe->AllocResourceSet(0);

	auto genShader = device->Create<rhi::Shader>();
	res = genShader->Load("data/gen.comp", rhi::ShaderKind::Compute);
	ASSERT(res);

	auto genPipe = device->Create<rhi::Pipeline>();
	res = genPipe->Init(std::span(&genShader, 1));
	ASSERT(res);

	rhi::ShaderParam const *uniformParam = genShader->GetParam(rhi::ShaderParam::UniformBuffer);
	ASSERT(uniformParam);

	auto genUniform = device->Create<rhi::Buffer>("gen Uniforms");
	res = genUniform->Init(rhi::ResourceDescriptor{
		._usage = rhi::ResourceUsage{.srv = 1, .cpuAccess = 1},
		._dimensions = glm::ivec4{(int32_t)uniformParam->_type->_size, 0, 0, 0},
	});
	ASSERT(res);

	{
		auto mapped = genUniform->Map();
		utl::AnyRef block{ uniformParam->_type, mapped.data() };
		auto *blockSize = block.GetMember("blockSize").Get<glm::ivec2>();
		*blockSize = glm::ivec2(16, 32);
		glm::vec4 *blockColors = block.GetMember("blockColors").GetArrayElement(0).Get<glm::vec4>();
		ASSERT(block.GetMember("blockColors").GetArraySize() == 2);
		blockColors[0] = glm::vec4(1, 0, 0, 1);
		blockColors[1] = glm::vec4(0, 1, 0, 1);
		genUniform->Unmap();
	}

	std::shared_ptr<rhi::Texture> genOutput;
	std::shared_ptr<rhi::ResourceSet> genResources;

	rhi::PresentMode presentMode = window->_swapchain->_descriptor._presentMode;

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
				presentMode = (rhi::PresentMode)std::countr_zero(nextModes);
			}
		}
	};

	for (; running; ) {
		eng::Sys::Get()->_ui->HandleInput();
		eng::Sys::Get()->_ui->UpdateWindows();

		glm::ivec2 swapchainSize = glm::ivec2(window->_swapchain->_descriptor._dimensions);
		if (any(equal(swapchainSize, glm::ivec2(0))))
			continue;

		if (!genOutput || glm::ivec2(genOutput->_descriptor._dimensions) != swapchainSize) {
			genOutput = device->Create<rhi::Texture>("gen Output");
			res = genOutput->Init(rhi::ResourceDescriptor{
				._usage = rhi::ResourceUsage{.srv = 1, .uav = 1, .copySrc = 1, .copyDst = 1},
				._format = rhi::Format::R8G8B8A8,
				._dimensions{swapchainSize, 0, 0},
				._mipLevels = 0
			});
			ASSERT(res);

			genResources = genPipe->AllocResourceSet(0);
			res = genResources->Update({ {genUniform}, {genOutput} });
			ASSERT(res);

			res = solidResSet->Update({ {solidUniform}, {sampler}, {genOutput} });
			ASSERT(res);
		}

		std::vector<std::shared_ptr<rhi::Pass>> passes;

		auto genPass = device->Create<rhi::ComputePass>("gen");
		glm::ivec2 genGroup = genPipe->GetComputeGroupSize();
		res = genPass->Init(genPipe.get(), std::span(&genResources, 1), glm::ivec3((swapchainSize + genGroup - 1) / genGroup, 1));
		ASSERT(res);
		passes.push_back(genPass);

		auto mipGenPass = device->Create<rhi::CopyPass>();
		mipGenPass->CopyTopToLowerMips(genOutput);
		passes.push_back(mipGenPass);

		auto swapchainTexture = window->_swapchain->AcquireNextImage();
			
		//auto copyPass = device->Create<rhi::CopyPass>("genCopy");
		////res = copyPass->CopyMips(genOutput, swapchainTexture);
		//rhi::ResourceView genView = rhi::ResourceView::FromDescriptor(genOutput->_descriptor, 0, 1);
		//genView._region._max.x /= 2;
		//res = copyPass->Copy({ {genOutput, genView}, {swapchainTexture} });
		//ASSERT(res);
		//passes.push_back(copyPass);

		window->_imguiCtx->LayoutUi([] {
			ImGui::Begin("Fps", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground /* | ImGuiWindowFlags_AlwaysAutoResize */);
			ImGui::SetWindowPos(ImVec2(10, 10), ImGuiCond_Once);
			ImGui::SetWindowSize(ImVec2(100, 20), ImGuiCond_Once);
			ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
			ImGui::End();

			//ImGui::ShowDemoWindow();
		});


		std::array<rhi::GraphicsPass::TargetData, 1> renderTargets{
			{
				swapchainTexture,
				glm::vec4(0, 0, 1 ,1),
			},
		};
		ASSERT(renderTargets[0]._texture);
		auto renderPass = device->Create<rhi::GraphicsPass>("Render");
		res = renderPass->Init(renderTargets);
		ASSERT(res);
		rhi::GraphicsPass::BufferStream solidVerts{ triBuf };
		renderPass->Draw({
			._pipeline = solidPipe,
			._resourceSets = {std::span(&solidResSet, 1)},
			._vertexStreams = {std::span(&solidVerts, 1)},
			._indices = utl::IntervalU::FromMinAndSize(0, 3),
		});

		window->_imguiCtx->Render(renderPass.get());

		passes.push_back(renderPass);

		auto presentPass = device->Create<rhi::PresentPass>("Present");
		presentPass->SetSwapchainTexture(swapchainTexture);
		passes.push_back(presentPass);

		auto submission = device->Submit(std::move(passes), "Execute");
		res = submission->Prepare();
		ASSERT(res);
		res = submission->Execute();
		ASSERT(res);
		res = submission->WaitUntilFinished();
		ASSERT(res);
	}

	std::cout << "Bye!\n"; 

	return 0;
}