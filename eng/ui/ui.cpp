#include "ui.h"
#include "window.h"
#include "eng/sys.h"

#include "SDL2/SDL.h"


namespace eng {

utl::Transform3F TransformFromKeyboardInput(double deltaTime, float metersPerSec, float degreesPerSec)
{
	uint8_t const *keys = SDL_GetKeyboardState(nullptr);

	const float velocity = metersPerSec * deltaTime;
	const float angVelocity = degreesPerSec * glm::pi<float>() / 180 * deltaTime;

	utl::Transform3F xform;

	if (keys[SDL_SCANCODE_W])
		xform._position.z -= velocity;
	if (keys[SDL_SCANCODE_S])
		xform._position.z += velocity;
	if (keys[SDL_SCANCODE_A])
		xform._position.x -= velocity;
	if (keys[SDL_SCANCODE_D])
		xform._position.x += velocity;
	if (keys[SDL_SCANCODE_R])
		xform._position.y -= velocity;
	if (keys[SDL_SCANCODE_F])
		xform._position.y += velocity;
	if (keys[SDL_SCANCODE_Q])
		xform._orientation *= glm::angleAxis(+angVelocity, glm::vec3(0, 1, 0));
	if (keys[SDL_SCANCODE_E])
		xform._orientation *= glm::angleAxis(-angVelocity, glm::vec3(0, 1, 0));
	if (keys[SDL_SCANCODE_Z])
		xform._orientation *= glm::angleAxis(-angVelocity, glm::vec3(0, 0, 1));
	if (keys[SDL_SCANCODE_C])
		xform._orientation *= glm::angleAxis(+angVelocity, glm::vec3(0, 0, 1));
	if (keys[SDL_SCANCODE_T])
		xform._orientation *= glm::angleAxis(-angVelocity, glm::vec3(1, 0, 0));
	if (keys[SDL_SCANCODE_G])
		xform._orientation *= glm::angleAxis(+angVelocity, glm::vec3(1, 0, 0));

	return xform;
}

Ui::~Ui()
{
	SDL_Quit();
}

bool Ui::Init()
{
#if defined(__linux__)	
	setenv("SDL_VIDEODRIVER", "x11", 1);
#endif	

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		return false;

	return true;
}

std::shared_ptr<Window> Ui::NewWindow(WindowDescriptor const &winDesc)
{
	auto window = std::make_shared<Window>();
	if (!window->Init(winDesc))
		return nullptr;

	return window;
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
#if SDL_VERSION_ATLEAST(2, 0, 22)				
			case SDL_TEXTEDITING_EXT:
				windowId = event.editExt.windowID;
				break;
#endif				
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
	ASSERT(!window->_swapchain);
	if (Sys::Get()->_rhi) {
		bool res = window->InitRendering();
		ASSERT(res);
	}
}

void Ui::UnregisterWindow(Window *window)
{
	_windows.erase(std::find(_windows.begin(), _windows.end(), window));
}

}