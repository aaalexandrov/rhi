#include "ui.h"
#include "window.h"

#include "SDL2/SDL.h"


namespace eng {

Ui::~Ui()
{
	SDL_Quit();
}

bool Ui::Init()
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		return false;

	return true;
}

void Ui::HandleInput()
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {

		uint32_t windowId = 0;
		switch (event.type) {
			case SDL_WINDOWEVENT:
				windowId = event.window.windowID;
				break;
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				windowId = event.key.windowID;
				break;
			case SDL_TEXTEDITING:
				windowId = event.edit.windowID;
				break;
			case SDL_TEXTEDITING_EXT:
				windowId = event.editExt.windowID;
				break;
			case SDL_TEXTINPUT:
				windowId = event.text.windowID;
				break;
			case SDL_MOUSEMOTION:
				windowId = event.motion.windowID;
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				windowId = event.button.windowID;
				break;
			case SDL_MOUSEWHEEL:
				windowId = event.wheel.windowID;
				break;
			case SDL_FINGERMOTION:
			case SDL_FINGERDOWN:
			case SDL_FINGERUP:
				windowId = event.tfinger.windowID;
				break;
			case SDL_DROPBEGIN:
			case SDL_DROPFILE:
			case SDL_DROPTEXT:
			case SDL_DROPCOMPLETE:
				windowId = event.drop.windowID;
				break;
			default:
				if (event.type >= SDL_USEREVENT)
					windowId = event.user.windowID;
				break;
		}

		Window *targetWindow = nullptr;
		if (windowId) {
			for (Window *win : _windows) {
				if (SDL_GetWindowID(win->_window) == windowId) {
					targetWindow = win;
					break;
				}
			}
			if (targetWindow) {
				targetWindow->_imguiCtx->ProcessEvent(targetWindow, event);
			}
		} else {
			for (Window *win : _windows) {
				win->_imguiCtx->ProcessEvent(targetWindow, event);
			}
		}

	}

}

void Ui::UpdateWindows()
{
	for (Window *win : _windows) {
		win->_swapchain->Update();
	}
}

void Ui::RegisterWindow(Window *window)
{
	_windows.push_back(window);
}

void Ui::UnregisterWindow(Window *window)
{
	_windows.erase(std::find(_windows.begin(), _windows.end(), window));
}

}