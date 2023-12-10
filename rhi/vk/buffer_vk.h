#pragma once

#include "base_vk.h"
#include "../resource.h"

namespace rhi {

struct BufferVk : public Buffer {


	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<BufferVk>(); }

	vk::Buffer _buffer;
};

}