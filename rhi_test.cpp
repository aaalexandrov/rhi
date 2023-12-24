#include <iostream>

#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"

#include "rhi/vk/rhi_vk.h"
#include "rhi/pass.h"
#include "rhi/pipeline.h"

int main()
{
	utl::TypeInfo::Init();
	utl::OnDestroy typesDone(utl::TypeInfo::Done);

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cout << "Failed to initialize SDL\n";
		return -1;
	}

	SDL_Window *window = SDL_CreateWindow("Testing", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (!window) {
		std::cout << "Failed to create window\n";
		return -1;
	}

	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(window, &wmInfo);

	rhi::Rhi::Settings deviceSettings{
		._appName = "RhiTest",
		._appVersion = glm::uvec3(0, 1, 0),
#if !defined(NDEBUG)
		._enableValidation = true,
#endif
	};

#if defined(_WIN32)
	deviceSettings._window = std::shared_ptr<rhi::WindowData>(
		new rhi::WindowDataWin32{ wmInfo.info.win.hinstance, wmInfo.info.win.window }
	);
#elif defined(__linux__)	
	deviceSettings._window = std::shared_ptr<rhi::WindowData>(
		new rhi::WindowDataXlib{ wmInfo.info.x11.display,  wmInfo.info.x11.window }
	);
#else
#	error Unsupported platform!
#endif

	auto device = std::make_shared<rhi::RhiVk>();
	bool res = device->Init(deviceSettings);
	ASSERT(res);

	auto swapchain = device->Create<rhi::Swapchain>("Swapchain");
	rhi::SwapchainDescriptor chainDesc{
		._usage{.rt = 1, .copySrc = 1, .copyDst = 1},
		._format = rhi::Format::B8G8R8A8_srgb,
		._presentMode = rhi::PresentMode::Fifo,
		._window = deviceSettings._window,
	};
	res = swapchain->Init(chainDesc);
	ASSERT(res);

	auto solidVert = device->Create<rhi::Shader>();
	res = solidVert->Load("data/solid.vert", rhi::ShaderKind::Vertex);
	ASSERT(res);

	auto solidFrag = device->Create<rhi::Shader>();
	res = solidFrag->Load("data/solid.frag", rhi::ShaderKind::Fragment);
	ASSERT(res);

	std::array<rhi::GraphicsPass::TargetData, 1> solidRts{{	swapchain->_images[0] }};
	auto solidPass = device->Create<rhi::GraphicsPass>("solidPass", std::span(solidRts));

	auto solidPipe = device->Create<rhi::Pipeline>();
	rhi::GraphicsPipelineData solidData{
		._shaders = {{ solidVert, solidFrag }},
		._renderPass = std::move(solidPass),
		._vertexInputs = { rhi::VertexInputData{ ._layout = solidVert->GetParam(rhi::ShaderParam::Kind::VertexLayout, 0)->_ownTypes[0] }},
	};
	res = solidPipe->Init(solidData);
	ASSERT(res);

	auto *vertLayout = solidPipe->GetShader(rhi::ShaderKind::Vertex)->GetParam(rhi::ShaderParam::VertexLayout);
	auto triBuf = device->Create<rhi::Buffer>("triangle", rhi::ResourceDescriptor{
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
	auto solidUniform = device->Create<rhi::Buffer>("solidUniform", rhi::ResourceDescriptor{
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

	auto sampler = device->Create<rhi::Sampler>("samp", rhi::SamplerDescriptor{});

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

	for (bool running = true; running; ) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				running = false;
			}

			res = swapchain->Update();
			if (!res)
				continue;

			glm::ivec2 swapchainSize = glm::ivec2(swapchain->_descriptor._dimensions);
			if (!genOutput || glm::ivec2(genOutput->_descriptor._dimensions) != swapchainSize) {
				genOutput = device->Create<rhi::Texture>("gen Output");
				res = genOutput->Init(rhi::ResourceDescriptor{
					._usage = rhi::ResourceUsage{.srv = 1, .uav = 1, .copySrc = 1},
					._format = rhi::Format::R8G8B8A8,
					._dimensions{swapchainSize, 0, 0},
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

			auto swapchainTexture = swapchain->AcquireNextImage();
			
			auto copyPass = device->Create<rhi::CopyPass>("genCopy");
			res = copyPass->Copy({ {genOutput}, {swapchainTexture} });
			ASSERT(res);
			passes.push_back(copyPass);


			//std::array<rhi::GraphicsPass::TargetData, 1> renderTargets{
			//	{
			//		swapchainTexture,
			//		glm::vec4(0, 0, 1 ,1),
			//	},
			//};
			//ASSERT(renderTargets[0]._texture);
			//auto renderPass = device->Create<rhi::GraphicsPass>("Render");
			//res = renderPass->Init(renderTargets);
			//ASSERT(res);
			//rhi::GraphicsPass::BufferStream solidVerts{ triBuf };
			//renderPass->Draw({
			//	._pipeline = solidPipe,
			//	._resourceSets = {std::span(&solidResSet, 1)},
			//	._vertexStreams = {std::span(&solidVerts, 1)},
			//	._indices = utl::IntervalU::FromMinAndSize(0, 3),
			//});
			//passes.push_back(renderPass);

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
	}

	std::cout << "Bye!\n"; 

	return 0;
}