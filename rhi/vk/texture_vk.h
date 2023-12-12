#pragma once

#include "base_vk.h"
#include "../resource.h"

namespace rhi {

struct TextureVk : public Texture {
	~TextureVk() override;

	bool Init(ResourceDescriptor const &desc) override;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<TextureVk>(); }

	vk::ImageLayout GetInitialLayout() const;

	vk::Image _image;
	VmaAllocation _vmaAlloc = {};
};

}