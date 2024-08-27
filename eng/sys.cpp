#include "sys.h"
#include "world.h"
#include "render/scene.h"
#include "rhi/vk/rhi_vk.h"

namespace eng {
Sys::~Sys()
{
}

bool Sys::Init()
{
    _ui = std::make_unique<Ui>();
    if (!_ui->Init())
        return false;

    _lastUpdateTime = std::chrono::high_resolution_clock::now();

    return true;
}

bool Sys::InitRhi(std::shared_ptr<Window> const &window, int32_t deviceIndex)
{
    _rhi = std::static_pointer_cast<rhi::Rhi>(std::make_shared<rhi::RhiVk>());

    rhi::Rhi::Settings rhiSettings{
        ._appName = window->_desc._name.c_str(),
        ._appVersion = glm::uvec3(0, 1, 0),
#if !defined(NDEBUG)
        ._enableValidation = true,
#endif
        ._window = window->GetWindowData(),
    };

    if (!_rhi->Init(rhiSettings, deviceIndex))
        return false;

    LOG("Created rhi device '%s'", _rhi->GetInitializedDevice()._name);

    for (auto *win : _ui->_windows) {
        ASSERT(!win->_swapchain);
        if (!win->InitRendering())
            return false;
    }

    return true;
}

void Sys::UpdateTime()
{
    std::chrono::steady_clock::time_point now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<utl::UpdateQueue::Time> deltaSec = now - _lastUpdateTime;

    double delta = deltaSec.count() * _timeScale;
    _updateQueue.UpdateToTime(delta);

    if (_world)
        _world->UpdateTime(delta);

    _lastUpdateTime = now;
}


bool Sys::InitInstance()
{
    ASSERT(!s_instance);
    s_instance = std::make_unique<Sys>();
    if (!s_instance->Init())
        return false;
    return true;
}

void Sys::DoneInstance()
{
    s_instance = nullptr;
}

}