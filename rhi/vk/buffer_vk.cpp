#include "buffer_vk.h"
#include "rhi_vk.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("buffer_vk", [] {
	TypeInfo::Register<BufferVk>().Name("BufferVk")
		.Base<Buffer>()
		.Base<ResourceVk>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());
});


BufferVk::~BufferVk()
{
	auto rhi = static_cast<RhiVk*>(_rhi);
	vmaDestroyBuffer(rhi->_vma, _buffer, _vmaAlloc);
}

vk::BufferUsageFlags GetBufferUsage(ResourceUsage usage)
{
	ASSERT(!usage.rt);
	ASSERT(!usage.present);
	vk::BufferUsageFlags flags;
	if (usage.vb)
		flags |= vk::BufferUsageFlagBits::eVertexBuffer;
	if (usage.ib)
		flags |= vk::BufferUsageFlagBits::eIndexBuffer;
	if (usage.srv)
		flags |= vk::BufferUsageFlagBits::eUniformBuffer;
	if (usage.uav)
		flags |= vk::BufferUsageFlagBits::eStorageBuffer;
	if (usage.copySrc)
		flags |= vk::BufferUsageFlagBits::eTransferSrc;
	if (usage.copyDst)
		flags |= vk::BufferUsageFlagBits::eTransferDst;

	return flags;
}

bool BufferVk::Init(ResourceDescriptor const &desc)
{
	if (!Buffer::Init(desc))
		return false;

	auto rhi = static_cast<RhiVk*>(_rhi);
	auto queueFamilies = rhi->GetQueueFamilyIndices(_descriptor._usage);
	vk::BufferCreateInfo bufInfo{
		vk::BufferCreateFlags(),
		GetSize(),
		GetBufferUsage(_descriptor._usage),
		vk::SharingMode::eExclusive,
		(uint32_t)queueFamilies.size(),
		queueFamilies.data(),
	};
	VmaAllocationCreateInfo allocInfo = rhi->GetVmaAllocCreateInfo(this);
	if ((vk::Result)vmaCreateBuffer(rhi->_vma, (VkBufferCreateInfo*)&bufInfo, &allocInfo, (VkBuffer*)&_buffer, &_vmaAlloc, nullptr) != vk::Result::eSuccess)
		return false;

	rhi->SetDebugName(vk::ObjectType::eBuffer, (uint64_t)(VkBuffer)_buffer, _name.c_str());

	return true;
}

std::span<uint8_t> BufferVk::Map()
{
	auto rhi = static_cast<RhiVk*>(_rhi);
	void *mapped = nullptr;
	if ((vk::Result)vmaMapMemory(rhi->_vma, _vmaAlloc, &mapped) != vk::Result::eSuccess)
		return std::span<uint8_t>();
	return std::span((uint8_t *)mapped, GetSize());
}

bool BufferVk::Unmap()
{
	auto rhi = static_cast<RhiVk*>(_rhi);
	vmaUnmapMemory(rhi->_vma, _vmaAlloc);
	return true;
}

ResourceTransitionVk BufferVk::GetTransitionData(ResourceUsage prevUsage, ResourceUsage usage)
{
	ResourceTransitionVk data{
		._srcState = GetState(prevUsage),
		._dstState = GetState(usage),
	};
	return data;
}

ResourceStateVk BufferVk::GetState(ResourceUsage usage)
{
	ResourceStateVk state;
	state._access = GetAccess(usage);
	state._stages = GetPipelineStages(usage);
	return state;
}

}