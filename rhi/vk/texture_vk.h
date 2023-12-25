#pragma once

#include "base_vk.h"
#include "../resource.h"

namespace rhi {

struct TextureVk : public Texture, public ResourceVk {
	~TextureVk() override;

	bool Init(ResourceDescriptor const &desc) override;
	bool Init(vk::Image image, ResourceDescriptor &desc, RhiOwned *owner);

	ResourceTransitionVk GetTransitionData(ResourceUsage prevUsage, ResourceUsage usage) override;
	ResourceStateVk GetState(ResourceUsage usage);

	vk::ImageView CreateView(ResourceView const &view);

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<TextureVk>(); }

	vk::ImageLayout GetInitialLayout() const;

	vk::Image _image;
	VmaAllocation _vmaAlloc = {};
	vk::ImageView _view;
};

vk::ImageUsageFlags GetImageUsage(ResourceUsage usage, Format imgFormat);
vk::ImageLayout GetImageLayout(ResourceUsage usage);
vk::ImageAspectFlags GetImageAspect(Format fmt);
vk::ImageType GetImageType(glm::ivec4 dims);
vk::ImageViewType GetImageViewType(glm::ivec4 dims);
vk::ImageSubresourceRange GetViewSubresourceRange(ResourceView const &view);

}