#pragma once

#include "base_vk.h"
#include "../resource.h"

namespace rhi {

struct BufferVk : public Buffer {
	~BufferVk() override;

	bool Init(ResourceDescriptor const &desc) override;

	std::span<uint8_t> Map(size_t offset = 0, size_t size = ~0ull) override;
	bool Unmap() override;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<BufferVk>(); }

	vk::Buffer _buffer;
	VmaAllocation _vmaAlloc = {};
};

}