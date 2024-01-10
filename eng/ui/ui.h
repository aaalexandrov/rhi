#pragma once

namespace rhi {
struct Rhi;
}

namespace eng {

struct Window;
struct Ui {
	~Ui();

	bool Init();

	void HandleInput();

	void UpdateWindows();

	void RegisterWindow(Window *window);
	void UnregisterWindow(Window *window);

	std::vector<Window *> _windows;
};

}