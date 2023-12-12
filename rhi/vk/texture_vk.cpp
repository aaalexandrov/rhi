#include "texture_vk.h"
#include "rhi_vk.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("texture_vk", [] {
	TypeInfo::Register<TextureVk>().Name("TextureVk")
		.Base<Texture>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());
});


TextureVk::~TextureVk()
{
	auto rhi = static_pointer_cast<RhiVk>(_rhi.lock());
	vmaDestroyImage(rhi->_vma, _image, _vmaAlloc);
}

vk::ImageCreateFlags GetImageCreateFlags(ResourceDescriptor const &desc)
{
	vk::ImageCreateFlags flags = {};
	if (desc._usage.cube)
		flags |= vk::ImageCreateFlagBits::eCubeCompatible;
	return flags;
}

vk::ImageType GetImageType(glm::uvec4 dims)
{
	ASSERT(dims[0] > 0);
	if (dims[1] == 0)
		return vk::ImageType::e1D;
	if (dims[2] == 0)
		return vk::ImageType::e2D;
	return vk::ImageType::e3D;
}

vk::Extent3D GetExtent3D(glm::vec3 dim)
{
	return vk::Extent3D(dim[0], dim[1], dim[2]);
}

vk::ImageUsageFlags GetImageUsage(ResourceDescriptor desc)
{
	vk::ImageUsageFlags imgUsage;
	if (desc._usage.copySrc)
		imgUsage |= vk::ImageUsageFlagBits::eTransferSrc;
	if (desc._usage.copyDst)
		imgUsage |= vk::ImageUsageFlagBits::eTransferDst;
	if (desc._usage.srv)
		imgUsage |= vk::ImageUsageFlagBits::eSampled;
	if (desc._usage.uav)
		imgUsage |= vk::ImageUsageFlagBits::eStorage;
	if (desc._usage.rt)
		imgUsage |= (Format::DepthStencilFirst <= desc._format && desc._format <= Format::DepthStencilLast) 
			? vk::ImageUsageFlagBits::eDepthStencilAttachment 
			: vk::ImageUsageFlagBits::eColorAttachment;
	return imgUsage;
}

vk::ImageLayout TextureVk::GetInitialLayout() const
{
	if (_descriptor._usage.cpuAccess && _descriptor._usage.copySrc)
		return vk::ImageLayout::ePreinitialized;
	// to do: choose a concrete layout
	return vk::ImageLayout::eUndefined;
}


bool TextureVk::Init(ResourceDescriptor const &desc)
{
	auto rhi = static_pointer_cast<RhiVk>(_rhi.lock()); 
	auto queueFamilies = rhi->GetQueueFamilyIndices(_descriptor._usage);
	vk::ImageCreateInfo imgInfo{
		GetImageCreateFlags(_descriptor),
		GetImageType(_descriptor._dimensions),
		s_vk2Format.ToSrc(_descriptor._format, vk::Format::eUndefined),
		GetExtent3D(_descriptor._dimensions),
		_descriptor._mipLevels,
		_descriptor._dimensions[3],
		vk::SampleCountFlagBits::e1,
		_descriptor._usage.cpuAccess ? vk::ImageTiling::eLinear : vk::ImageTiling::eOptimal,
		GetImageUsage(_descriptor),
		vk::SharingMode::eExclusive,
		(uint32_t)queueFamilies.size(),
		queueFamilies.data(),
		GetInitialLayout(),
	};
	VmaAllocationCreateInfo allocInfo = rhi->GetVmaAllocCreateInfo(this);
	if ((vk::Result)vmaCreateImage(rhi->_vma, (VkImageCreateInfo *)&imgInfo, &allocInfo, (VkImage *)&_image, &_vmaAlloc, nullptr) != vk::Result::eSuccess)
		return false;

	return true;
}

}