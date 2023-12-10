#include <iostream>

#define SDL_MAIN_HANDLED
#include "sdl2/SDL.h"

#include "rhi/vk/rhi_vk.h"

int main()
{
	utl::TypeInfo::Init();
	utl::OnDestroy typesDone(utl::TypeInfo::Done);

	auto device = std::make_shared<rhi::RhiVk>();
	rhi::Rhi::Settings deviceSettings{
		._appName = "RhiTest",
		._appVersion = glm::uvec3(0, 1, 0),
#if !defined(NDEBUG)
		._enableValidation = true,
#endif
	};
	bool res = device->Init(deviceSettings);
	ASSERT(res);
	utl::OnDestroy deviceDone([&] { device->Done(); });

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cout << "Failed to initialize SDL\n";
		return -1;
	}

	SDL_Window *window = SDL_CreateWindow("Testing", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_SHOWN);
	if (!window) {
		std::cout << "Failed to create window\n";
		return -1;
	}

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