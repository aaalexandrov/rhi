#pragma once

#include "base_vk.h"
#include "../resource.h"

namespace rhi {

struct TextureVk : public Texture {
	~TextureVk() override;

	bool Init(ResourceDescriptor const &desc) override;
	bool Init(vk::Image image, ResourceDescriptor &desc, RhiOwned *owner);

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<TextureVk>(); }

	vk::ImageLayout GetInitialLayout() const;

	vk::Image _image;
	VmaAllocation _vmaAlloc = {};
	std::weak_ptr<RhiOwned> _owner;
};

vk::ImageUsageFlags GetImageUsage(ResourceUsage usage, Format imgFormat);

}