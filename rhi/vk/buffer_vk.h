#pragma once

#include "base_vk.h"
#include "../resource.h"

namespace rhi {

struct BufferVk : public Buffer {
	~BufferVk() override;

	bool Init(ResourceDescriptor const &desc) override;

	std::span<uint8_t> Map() override;
	bool Unmap() override;

	bool RecordTransition(vk::CommandBuffer cmds, ResourceUsage prevUsage, ResourceUsage usage);

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<BufferVk>(); }

	vk::Buffer _buffer;
	VmaAllocation _vmaAlloc = {};
};

}