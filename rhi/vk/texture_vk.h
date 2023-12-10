#pragma once

#include "base_vk.h"
#include "../resource.h"

namespace rhi {

struct TextureVk : public Texture {


	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<TextureVk>(); }

	vk::Image _image;
};

}