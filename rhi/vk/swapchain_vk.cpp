#include "swapchain_vk.h"
#include "rhi_vk.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("swapchain_vk", [] {
	TypeInfo::Register<SwapchainVk>().Name("SwapchainVk")
		.Base<Swapchain>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());
});


SwapchainVk::~SwapchainVk()
{
	_images.clear();

	auto rhi = static_pointer_cast<RhiVk>(_rhi.lock());
	rhi->_device.destroySwapchainKHR(_swapchain);
	rhi->_instance.destroySurfaceKHR(_surface);
	rhi->_device.destroySemaphore(_acquireSemaphore, rhi->AllocCallbacks());
}

bool SwapchainVk::Init(SwapchainDescriptor const &desc)
{
	if (!Swapchain::Init(desc))
		return false;

	auto rhi = static_pointer_cast<RhiVk>(_rhi.lock());

#if defined(_WIN32)
	WindowDataWin32 *winData = Cast<WindowDataWin32>(_descriptor._window.get());
	if (!winData)
		return false;
	vk::Win32SurfaceCreateInfoKHR surfInfo{
		vk::Win32SurfaceCreateFlagsKHR(),
		winData->_hinstance,
		winData->_hwnd
	};
	if (rhi->_instance.createWin32SurfaceKHR(&surfInfo, rhi->AllocCallbacks(), &_surface) != vk::Result::eSuccess)
		return false;
#elif defined(__linux__)
	WindowDataXlib *winData = Cast<WindowDataXlib>(_descriptor._window.get());
	if (!winData)
		return false;
	vk::XlibSurfaceCreateInfoKHR surfInfo{
		vk::XlibSurfaceCreateFlagsKHR(),
		createXlib->_display,
		createXlib->_window 
	};
	if (rhi->_instance.createXlibSurfaceKHR(&surfInfo, rhi->AllocCallbacks(), &_surface) != vk::Result::eSuccess)
		return false;
#endif
	{
		auto supported = rhi->_physDevice.getSurfaceSupportKHR(rhi->_universalQueue._family, _surface);
		if (supported.result != vk::Result::eSuccess || !supported.value)
			return false;
	}

	vk::SemaphoreCreateInfo semInfo{};
	if (rhi->_device.createSemaphore(&semInfo, rhi->AllocCallbacks(), &_acquireSemaphore) != vk::Result::eSuccess)
		return false;

	auto surfFormats = GetSupportedSurfaceFormats();
	if (surfFormats.empty())
		return false;
	if (std::find(surfFormats.begin(), surfFormats.end(), _descriptor._format) == surfFormats.end())
		_descriptor._format = surfFormats.front();

	uint32_t surfModes = GetSupportedPresentModeMask();
	if (!surfModes)
		return false;
	_descriptor._presentMode = (PresentMode)std::countr_zero(surfModes);
	ASSERT(surfModes & (1 << (uint32_t)_descriptor._presentMode));

	return true;
}

std::vector<Format> SwapchainVk::GetSupportedSurfaceFormats() const
{
	auto rhi = static_pointer_cast<RhiVk>(_rhi.lock());
	auto vkFormats = rhi->_physDevice.getSurfaceFormatsKHR(_surface).value;
	std::vector<Format> formats;
	for (auto &vkFmt : vkFormats) {
		formats.push_back(s_vk2Format.ToDst(vkFmt.format, rhi::Format::Invalid));
	}

	return formats;
}

uint32_t SwapchainVk::GetSupportedPresentModeMask() const
{
	auto rhi = static_pointer_cast<RhiVk>(_rhi.lock());
	auto vkModes = rhi->_physDevice.getSurfacePresentModesKHR(_surface).value;
	uint32_t modeMask = 0;
	for (auto &vkMode : vkModes) {
		PresentMode mode = s_vk2PresentMode.ToDst(vkMode, PresentMode::Invalid);
		if (mode != PresentMode::Invalid)
			modeMask |= 1 << (uint8_t)mode;
	}
	return modeMask;
}

bool SwapchainVk::Update(glm::uvec2 size, PresentMode presentMode, Format surfaceFormat)
{
	return false;
}

std::shared_ptr<Texture> SwapchainVk::AcquireNextImage()
{
	return std::shared_ptr<Texture>();
}

}