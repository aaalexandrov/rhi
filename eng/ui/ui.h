#pragma once

namespace rhi {
struct Rhi;
}

namespace eng {

struct WindowDescriptor;
struct Window;
struct Ui {
	~Ui();

	bool Init();

	std::shared_ptr<Window> NewWindow(WindowDescriptor const &winDesc);

	void HandleInput();

	void UpdateWindows();

	void RegisterWindow(Window *window);
	void UnregisterWindow(Window *window);

	std::vector<Window *> _windows;
	bool _keyboardFocusInUI = false;
};

utl::Transform3F TransformFromKeyboardInput(double deltaTime, float metersPerSec = 6, float degreesPerSec = 120);

}