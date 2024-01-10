#include "window.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"

#include "rhi/vk/rhi_vk.h"
#include "eng/sys.h"
#include "eng/ui/imgui_ctx.h"

namespace eng {

static auto s_regTypes = TypeInfo::AddInitializer("window", [] {
    TypeInfo::Register<Window>().Name("eng::Window")
        .Base<utl::Any>();
});


Window::~Window()
{
    Done();
}

bool Window::Init(WindowDescriptor const &desc)
{
    ASSERT(!_window);
    _desc = desc;
    _window = SDL_CreateWindow(
        _desc._name.c_str(),
        _desc._pos.x >= 0 ? _desc._pos.x : SDL_WINDOWPOS_UNDEFINED,
        _desc._pos.y >= 0 ? _desc._pos.y : SDL_WINDOWPOS_UNDEFINED,
        _desc._size.x,
        _desc._size.y,
        SDL_WINDOW_SHOWN |
        (_desc._resizeable ? SDL_WINDOW_RESIZABLE : 0) |
        (_desc._borderless ? SDL_WINDOW_BORDERLESS : 0) |
        (_desc._fullscreen ? SDL_WINDOW_FULLSCREEN : 0)
    );
    if (!_window)
        return false;

    _desc._swapchainDesc._window = GetWindowData();

    Sys::Get()->_ui->RegisterWindow(this);

    return true;
}

bool Window::InitRendering()
{
    ASSERT(!_swapchain);
    ASSERT(!_imguiCtx);
    _swapchain = Sys::Get()->_rhi->New<rhi::Swapchain>(_desc._name, _desc._swapchainDesc);
    if (!_swapchain) 
        return false;

    _imguiCtx = std::make_unique<ImguiCtx>();
    if (!_imguiCtx->Init(this))
        return false;

    return true;
}

void Window::Done()
{
    Sys::Get()->_ui->UnregisterWindow(this);
    DoneRendering();
    if (_window) {
        SDL_DestroyWindow(_window);
        _window = nullptr;
    }
}

void Window::DoneRendering()
{
    _imguiCtx = nullptr;
    _swapchain = nullptr;
}

std::shared_ptr<rhi::WindowData> Window::GetWindowData()
{
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(_window, &wmInfo);

    std::shared_ptr<rhi::WindowData> windowData = 
#if defined(_WIN32)
    std::shared_ptr<rhi::WindowData>(
        new rhi::WindowDataWin32{ wmInfo.info.win.hinstance, wmInfo.info.win.window }
    );
#elif defined(__linux__)	
    std::shared_ptr<rhi::WindowData>(
        new rhi::WindowDataXlib{ wmInfo.info.x11.display,  wmInfo.info.x11.window }
    );
#else
#	error Unsupported platform!
#endif

    return windowData;
}

}