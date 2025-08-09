#pragma once

#include "base_vk.h"
#include "texture_vk.h"

namespace rhi {

struct SwapchainVk final : Swapchain {
	~SwapchainVk() override;

	bool Init(SwapchainDescriptor const &desc) override;

	std::vector<Format> GetSupportedSurfaceFormats() const override;
	uint32_t GetSupportedPresentModeMask() const override;

	bool Update(glm::ivec2 surfaceSize, PresentMode presentMode = PresentMode::Invalid, Format surfaceFormat = Format::Invalid) override;
	bool CreateSemaphores(uint32_t num);
	void DestroySemaphores();

	std::shared_ptr<Texture> AcquireNextImage() override;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<SwapchainVk>(); }

	vk::SurfaceKHR _surface;
	vk::SwapchainKHR _swapchain;
	std::vector<vk::Semaphore> _acquireSemaphores;
	bool _needsUpdate = false;
};

}