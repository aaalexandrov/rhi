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

	//auto buf = device->Create<rhi::Buffer>("Buf1");
	//rhi::ResourceDescriptor bufDesc{
	//	._usage = { .copySrc = 1, .cpuAccess = 1, },
	//	._dimensions = glm::uvec4(256, 0, 0, 0),
	//};
	//res = buf->Init(bufDesc);
	//ASSERT(res);

	//auto img = device->Create<rhi::Texture>("Tex1");
	//rhi::ResourceDescriptor imgDesc{
	//	._usage = { .srv = 1, .copyDst = 1, },
	//	._format = rhi::Format::R8G8B8A8,
	//	._dimensions = glm::uvec4(256, 256, 0, 0),
	//};
	//res = img->Init(imgDesc);
	//ASSERT(res);

	//auto samp = device->Create<rhi::Sampler>("Samp1");
	//rhi::SamplerDescriptor sampDesc;
	//res = samp->Init(sampDesc);
	//ASSERT(res);

	//auto shadVert = device->Create<rhi::Shader>();
	//res = shadVert->Load("data/solid.vert.spv", rhi::ShaderKind::Vertex);
	//ASSERT(res);

	//auto shadFrag = device->Create<rhi::Shader>();
	//res = shadFrag->Load("data/solid.frag", rhi::ShaderKind::Fragment);
	//ASSERT(res);

	auto shadComp = device->Create<rhi::Shader>();
	res = shadComp->Load("data/gen.comp", rhi::ShaderKind::Compute);
	ASSERT(res);


	auto pipeComp = device->Create<rhi::Pipeline>();
	res = pipeComp->Init(std::span(&shadComp, 1));
	ASSERT(res);

	pipeComp = nullptr;
	shadComp = nullptr;
	//shadFrag = nullptr;
	//shadVert = nullptr;
	//samp = nullptr;
	//img = nullptr;
	//buf = nullptr;

	auto swapchain = device->Create<rhi::Swapchain>("Swapchain");
	rhi::SwapchainDescriptor chainDesc{
		._usage{.rt = 1},
		._format = rhi::Format::B8G8R8A8_srgb,
		._presentMode = rhi::PresentMode::Fifo,
		._window = deviceSettings._window,
	};
	res = swapchain->Init(chainDesc);
	ASSERT(res);

	for (bool running = true; running; ) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				running = false;
			}

			res = swapchain->Update();
			if (!res)
				continue;

			std::vector<std::shared_ptr<rhi::Pass>> passes;
			std::array<rhi::GraphicsPass::TargetData, 1> renderTargets{
				{
					swapchain->AcquireNextImage(),
					glm::vec4(0, 0, 1 ,1),
				},
			};
			ASSERT(renderTargets[0]._texture);
			auto renderPass = device->Create<rhi::GraphicsPass>("Render");
			res = renderPass->Init(renderTargets);
			ASSERT(res);
			passes.push_back(renderPass);

			auto presentPass = device->Create<rhi::PresentPass>("Present");
			presentPass->SetSwapchainTexture(renderTargets[0]._texture);
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