#pragma once

#include "base_vk.h"
#include "texture_vk.h"

namespace rhi {

struct SwapchainVk : public Swapchain {
	~SwapchainVk() override;

	bool Init(SwapchainDescriptor const &desc) override;

	std::vector<Format> GetSupportedSurfaceFormats() const override;
	uint32_t GetSupportedPresentModeMask() const override;

	bool Update(glm::uvec2 size, PresentMode presentMode = PresentMode::Invalid, Format surfaceFormat = Format::Invalid) override;

	std::shared_ptr<Texture> AcquireNextImage() override;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<SwapchainVk>(); }

	vk::SurfaceKHR _surface;
	vk::SwapchainKHR _swapchain;
	vk::Semaphore _acquireSemaphore;
	std::vector<std::shared_ptr<TextureVk>> _images;
};

}