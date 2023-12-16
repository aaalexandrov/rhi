#include "buffer_vk.h"
#include "rhi_vk.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("buffer_vk", [] {
	TypeInfo::Register<BufferVk>().Name("BufferVk")
		.Base<Buffer>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());
});


BufferVk::~BufferVk()
{
	auto rhi = static_pointer_cast<RhiVk>(_rhi.lock());
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

	auto rhi = static_pointer_cast<RhiVk>(_rhi.lock());
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

	return true;
}

std::span<uint8_t> BufferVk::Map()
{
	auto rhi = static_pointer_cast<RhiVk>(_rhi.lock());
	void *mapped = nullptr;
	if ((vk::Result)vmaMapMemory(rhi->_vma, _vmaAlloc, &mapped) != vk::Result::eSuccess)
		return std::span<uint8_t>();
	return std::span((uint8_t *)mapped, GetSize());
}

bool BufferVk::Unmap()
{
	auto rhi = static_pointer_cast<RhiVk>(_rhi.lock());
	vmaUnmapMemory(rhi->_vma, _vmaAlloc);
	return true;
}

bool BufferVk::RecordTransition(vk::CommandBuffer cmds, ResourceUsage prevUsage, ResourceUsage usage)
{
	return false;
}

}