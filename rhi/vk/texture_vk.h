#pragma once

#include "base_vk.h"
#include "../resource.h"

namespace rhi {

struct TextureVk : public Texture {
	~TextureVk() override;

	bool Init(ResourceDescriptor const &desc) override;
	bool Init(vk::Image image, ResourceDescriptor &desc, RhiOwned *owner);

	bool InitView();

	bool RecordTransition(vk::CommandBuffer cmds, ResourceUsage prevUsage, ResourceUsage usage);

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<TextureVk>(); }

	vk::ImageLayout GetInitialLayout() const;

	vk::Image _image;
	VmaAllocation _vmaAlloc = {};
	vk::ImageView _view;
};

vk::ImageUsageFlags GetImageUsage(ResourceUsage usage, Format imgFormat);
vk::ImageLayout GetImageLayout(ResourceUsage usage);
vk::ImageAspectFlags GetImageAspect(Format fmt);
vk::ImageType GetImageType(glm::uvec4 dims);
vk::ImageViewType GetImageViewType(ResourceDescriptor const &desc);

}