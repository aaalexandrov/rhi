#include <iostream>

#define SDL_MAIN_HANDLED
#include "sdl2/SDL.h"
#include "SDL2/SDL_syswm.h"

#include "rhi/vk/rhi_vk.h"

int main()
{
	utl::TypeInfo::Init();
	utl::OnDestroy typesDone(utl::TypeInfo::Done);

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cout << "Failed to initialize SDL\n";
		return -1;
	}

	SDL_Window *window = SDL_CreateWindow("Testing", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_SHOWN);
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

	auto buf = device->Create<rhi::Buffer>("Test1");
	rhi::ResourceDescriptor bufDesc{
		._usage = { .copySrc = 1, .cpuAccess = 1, },
		._dimensions = glm::uvec4(256, 0, 0, 0),
	};
	res = buf->Init(bufDesc);
	ASSERT(res);

	buf = nullptr;

	for (bool running = true; running; ) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				running = false;
			}
		}
	}

	std::cout << "Bye!\n"; 

	return 0;
}