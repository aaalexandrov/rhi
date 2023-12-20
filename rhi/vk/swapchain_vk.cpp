#include "swapchain_vk.h"
#include "rhi_vk.h"
#include "utl/mathutl.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("swapchain_vk", [] {
	TypeInfo::Register<SwapchainVk>().Name("SwapchainVk")
		.Base<Swapchain>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());
});


SwapchainVk::~SwapchainVk()
{
	_images.clear();
	DestroySemaphores();

	auto rhi = static_cast<RhiVk*>(_rhi);
	rhi->_device.destroySwapchainKHR(_swapchain, rhi->AllocCallbacks());
	rhi->_instance.destroySurfaceKHR(_surface, rhi->AllocCallbacks());
}

bool SwapchainVk::Init(SwapchainDescriptor const &desc)
{
	if (!Swapchain::Init(desc))
		return false;

	auto rhi = static_cast<RhiVk*>(_rhi);

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
		winData->_display,
		winData->_window 
	};
	if (rhi->_instance.createXlibSurfaceKHR(&surfInfo, rhi->AllocCallbacks(), &_surface) != vk::Result::eSuccess)
		return false;
#endif
	{
		auto supported = rhi->_physDevice.getSurfaceSupportKHR(rhi->_universalQueue._family, _surface);
		if (supported.result != vk::Result::eSuccess || !supported.value)
			return false;
	}

	auto surfFormats = GetSupportedSurfaceFormats();
	if (surfFormats.empty())
		return false;
	if (std::find(surfFormats.begin(), surfFormats.end(), _descriptor._format) == surfFormats.end())
		_descriptor._format = surfFormats.front();

	uint32_t surfModes = GetSupportedPresentModeMask();
	if (!surfModes)
		return false;
	if ((surfModes & (1 << (uint32_t)_descriptor._presentMode)) == 0) {
		_descriptor._presentMode = (PresentMode)std::countr_zero(surfModes);
	}
	ASSERT(surfModes & (1 << (uint32_t)_descriptor._presentMode));

	return Update();
}

std::vector<Format> SwapchainVk::GetSupportedSurfaceFormats() const
{
	auto rhi = static_cast<RhiVk*>(_rhi);
	auto vkFormats = rhi->_physDevice.getSurfaceFormatsKHR(_surface).value;
	std::vector<Format> formats;
	for (auto &vkFmt : vkFormats) {
		formats.push_back(s_vk2Format.ToDst(vkFmt.format, rhi::Format::Invalid));
	}

	return formats;
}

uint32_t SwapchainVk::GetSupportedPresentModeMask() const
{
	auto rhi = static_cast<RhiVk*>(_rhi);
	auto vkModes = rhi->_physDevice.getSurfacePresentModesKHR(_surface).value;
	uint32_t modeMask = 0;
	for (auto &vkMode : vkModes) {
		PresentMode mode = s_vk2PresentMode.ToDst(vkMode, PresentMode::Invalid);
		if (mode != PresentMode::Invalid)
			modeMask |= 1 << (uint8_t)mode;
	}
	return modeMask;
}

bool SwapchainVk::Update(PresentMode presentMode, Format surfaceFormat)
{
	auto rhi = static_cast<RhiVk*>(_rhi);
	vk::SurfaceCapabilitiesKHR surfCaps;
	if ((vk::Result)rhi->_physDevice.getSurfaceCapabilitiesKHR(_surface, &surfCaps) != vk::Result::eSuccess)
		return false;
	vk::Extent2D swapchainSize = surfCaps.currentExtent;
	ASSERT(surfCaps.currentExtent.width != ~0u && surfCaps.currentExtent.height != ~0u);
	swapchainSize.width = utl::Clamp(surfCaps.minImageExtent.width, surfCaps.maxImageExtent.width, swapchainSize.width);
	swapchainSize.height = utl::Clamp(surfCaps.minImageExtent.height, surfCaps.maxImageExtent.height, swapchainSize.height);
	uint32_t imgCount = utl::Clamp(surfCaps.minImageCount, surfCaps.maxImageCount, (uint32_t)_descriptor._dimensions[2]);

	if (presentMode == PresentMode::Invalid)
		presentMode = _descriptor._presentMode;
	if (surfaceFormat == Format::Invalid)
		surfaceFormat = _descriptor._format;

	glm::ivec4 newDims = glm::ivec4(swapchainSize.width, swapchainSize.height, imgCount, _descriptor._dimensions[3]);
	if (!_needsUpdate && _descriptor._dimensions == newDims && presentMode == _descriptor._presentMode && surfaceFormat == _descriptor._format)
		return true;

	_images.clear();
	DestroySemaphores();

	ResourceUsage imgUsage = s_vkImageUsage2ResourceUsage.ToDst((VkImageUsageFlags)surfCaps.supportedUsageFlags);
	imgUsage &= rhi->GetFormatImageUsage(surfaceFormat) & _descriptor._usage;
	imgUsage.present = 1;

	auto swapchain = [&] { 
		if (swapchainSize.width == 0 || swapchainSize.height == 0)
			return vk::ResultValue<vk::SwapchainKHR>{ vk::Result::eErrorNotPermittedKHR, vk::SwapchainKHR() };

		auto queueFamilies = rhi->GetQueueFamilyIndices(_descriptor._usage);
		vk::SwapchainCreateInfoKHR chainInfo{
			vk::SwapchainCreateFlagsKHR(),
			_surface,
			imgCount,
			s_vk2Format.ToSrc(surfaceFormat, vk::Format::eUndefined),
			vk::ColorSpaceKHR::eSrgbNonlinear,
			swapchainSize,
			(uint32_t)std::max(_descriptor._dimensions[3], 1),
			GetImageUsage(imgUsage, surfaceFormat),
			vk::SharingMode::eExclusive,
			(uint32_t)queueFamilies.size(),
			queueFamilies.data(),
			(surfCaps.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
				? vk::SurfaceTransformFlagBitsKHR::eIdentity
				: surfCaps.currentTransform,
			vk::CompositeAlphaFlagBitsKHR::eOpaque,
			s_vk2PresentMode.ToSrc(presentMode),
			false,
			_swapchain,
			nullptr
		};

		return rhi->_device.createSwapchainKHR(chainInfo, rhi->AllocCallbacks()); 
	}();
	rhi->_device.destroySwapchainKHR(_swapchain, rhi->AllocCallbacks());
	_swapchain = swapchain.value;
	if (swapchain.result != vk::Result::eSuccess) {
		_descriptor._dimensions = glm::ivec4(glm::ivec3(0), _descriptor._dimensions[3]);
		_needsUpdate = true;
		return false;
	}

	_descriptor._dimensions = newDims;
	_descriptor._format = surfaceFormat;
	_descriptor._presentMode = presentMode;

	auto swapchainImages = rhi->_device.getSwapchainImagesKHR(_swapchain);
	if (swapchainImages.result != vk::Result::eSuccess)
		return false;

	ResourceDescriptor imgDesc{
		._usage = imgUsage,
		._format = _descriptor._format,
		._dimensions = _descriptor._dimensions,
		._mipLevels = 1,
	};
	imgDesc._dimensions[2] = 0;
	for (auto img : swapchainImages.value) {
		auto image = rhi->Create<TextureVk>((_name.empty() ? "SwapchainImg" : _name) + std::to_string(_images.size()));
		if (!image->Init(img, imgDesc, this))
			return false;
		_images.push_back(image);
	}

	// we create one more semaphore than the number of images because when acquiring an image
	// we need to supply a non-signalled semaphore before knowhing which image we'll get
	// so at the end of the array we keep an extra semaphore that'll be guaranteed unused 
	if (!CreateSemaphores(_images.size() + 1))
		return false;

	_needsUpdate = false;

	return true;
}

bool SwapchainVk::CreateSemaphores(uint32_t num)
{
	auto rhi = static_cast<RhiVk*>(_rhi);

	for (uint32_t i = 0; i < num; ++i) {
		vk::SemaphoreCreateInfo semInfo{};
		auto semRes = rhi->_device.createSemaphore(semInfo, rhi->AllocCallbacks());
		if (semRes.result != vk::Result::eSuccess)
			return false;
		_acquireSemaphores.push_back(semRes.value);
	}

	return true;
}

void SwapchainVk::DestroySemaphores()
{
	auto rhi = static_cast<RhiVk*>(_rhi);
	for (auto sem : _acquireSemaphores) {
		rhi->_device.destroySemaphore(sem, rhi->AllocCallbacks());
	}
	_acquireSemaphores.clear();
}

std::shared_ptr<Texture> SwapchainVk::AcquireNextImage()
{
	ASSERT(!_needsUpdate);
	ASSERT(_acquireSemaphores.size() == _images.size() + 1);
	auto rhi = static_cast<RhiVk*>(_rhi);
	uint32_t imgIndex = ~0;
	// use the extra semaphore
	vk::Result result = rhi->_device.acquireNextImageKHR(_swapchain, ~0ull, _acquireSemaphores.back(), nullptr, &imgIndex);
	switch (result) {
		case vk::Result::eSuboptimalKHR:
			_needsUpdate = true;
		case vk::Result::eSuccess:
			break;
		case vk::Result::eErrorOutOfDateKHR:
			_needsUpdate = true;
		default:
			return std::shared_ptr<Texture>();
	}
	// then put the extra semaphore at the index of the image
	std::swap(_acquireSemaphores[imgIndex], _acquireSemaphores.back());
	ASSERT(imgIndex < _images.size());
	_images[imgIndex]->_state = _images[imgIndex]->_state & ResourceUsage{ .create = 1 } | ResourceUsage{ .present = 1, .write = 1 };
	return _images[imgIndex];
}

}