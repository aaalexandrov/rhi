#pragma once
#include "rhi/resource.h"
#include "eng/ui/imgui_ctx.h"

struct SDL_Window;

namespace rhi {
struct WindowData;
}

namespace eng {

using utl::TypeInfo;

struct Window : std::enable_shared_from_this<Window>, utl::Any {
	struct Descriptor {
		std::string _name;
		glm::ivec2 _pos{ -1 };
		glm::ivec2 _size{ 1280, 720 };
		bool _resizeable = true;
		bool _borderless = false;
		bool _fullscreen = false;
	};

	~Window();

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Window>(); }

	bool Init(Descriptor const &desc);
	bool InitRendering(rhi::SwapchainDescriptor swapchainDesc);

	void Done();
	void DoneRendering();

	std::shared_ptr<rhi::WindowData> GetWindowData();

	Descriptor _desc;
	SDL_Window *_window = nullptr;
	std::shared_ptr<rhi::Swapchain> _swapchain;
	std::unique_ptr<ImguiCtx> _imguiCtx;
};

}