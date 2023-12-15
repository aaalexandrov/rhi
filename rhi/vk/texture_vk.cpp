#include "texture_vk.h"
#include "rhi_vk.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("texture_vk", [] {
	TypeInfo::Register<TextureVk>().Name("TextureVk")
		.Base<Texture>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());
});


vk::ImageCreateFlags GetImageCreateFlags(ResourceDescriptor const &desc)
{
	vk::ImageCreateFlags flags = {};
	if (desc._usage.cube)
		flags |= vk::ImageCreateFlagBits::eCubeCompatible;
	return flags;
}

vk::ImageAspectFlags GetImageAspect(Format fmt)
{
	vk::ImageAspectFlags flags;
	if (IsDepth(fmt))
		flags |= vk::ImageAspectFlagBits::eDepth;
	if (IsStencil(fmt))
		flags |= vk::ImageAspectFlagBits::eStencil;
	if (!flags)
		flags = vk::ImageAspectFlagBits::eColor;
	return flags;
}

vk::ImageType GetImageType(glm::uvec4 dims)
{
	ASSERT(dims.x > 0);
	if (dims.y == 0)
		return vk::ImageType::e1D;
	if (dims.z == 0)
		return vk::ImageType::e2D;
	return vk::ImageType::e3D;
}

vk::ImageViewType GetImageViewType(ResourceDescriptor const &desc)
{
	auto imgType = GetImageType(desc._dimensions);
	bool isArray = desc._dimensions[4] > 0;
	switch (imgType) {
		case vk::ImageType::e1D:
			return isArray ? vk::ImageViewType::e1DArray : vk::ImageViewType::e1D;
		case vk::ImageType::e2D:
			if (desc._usage.cube)
				return isArray ? vk::ImageViewType::eCubeArray : vk::ImageViewType::eCube;
			return isArray ? vk::ImageViewType::e2DArray : vk::ImageViewType::e2D;
		default:
			ASSERT(imgType == vk::ImageType::e3D);
			return vk::ImageViewType::e3D;
	}
}

vk::ImageUsageFlags GetImageUsage(ResourceUsage usage, Format imgFormat)
{
	vk::ImageUsageFlags imgUsage;
	if (usage.copySrc)
		imgUsage |= vk::ImageUsageFlagBits::eTransferSrc;
	if (usage.copyDst)
		imgUsage |= vk::ImageUsageFlagBits::eTransferDst;
	if (usage.srv)
		imgUsage |= vk::ImageUsageFlagBits::eSampled;
	if (usage.uav)
		imgUsage |= vk::ImageUsageFlagBits::eStorage;
	if (usage.rt)
		imgUsage |= vk::ImageUsageFlagBits::eColorAttachment;
	if (usage.ds)
		imgUsage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
	return imgUsage;
}

vk::ImageLayout GetImageLayout(ResourceUsage usage)
{
	static constexpr ResourceUsage opMask{ .srv = 1, .uav = 1, .rt = 1, .ds = 1, .present = 1, .copySrc = 1, .copyDst = 1 };
	static constexpr ResourceUsage rwMask{ .read = 1, .write = 1 };
	ASSERT(std::popcount((usage & opMask)._flags) == 1);
	ASSERT(std::popcount((usage & rwMask)._flags) == 1);
	ASSERT((usage & ~(opMask | rwMask)) == 0);
	if (usage.srv)
		return vk::ImageLayout::eShaderReadOnlyOptimal;
	if (usage.rt)
		return vk::ImageLayout::eColorAttachmentOptimal;
	if (usage.ds)
		return usage.write ? vk::ImageLayout::eDepthStencilAttachmentOptimal : vk::ImageLayout::eDepthStencilReadOnlyOptimal;
	if (usage.uav)
		return vk::ImageLayout::eGeneral;
	if (usage.copySrc)
		return vk::ImageLayout::eTransferSrcOptimal;
	if (usage.copyDst)
		return vk::ImageLayout::eTransferSrcOptimal;
	if (usage.present)
		return vk::ImageLayout::ePresentSrcKHR;
	ASSERT(0);
	return vk::ImageLayout::eGeneral;
}

TextureVk::~TextureVk()
{
	if (_vmaAlloc) {
		auto rhi = static_pointer_cast<RhiVk>(_rhi.lock());
		rhi->_device.destroyImageView(_view, rhi->AllocCallbacks());
		vmaDestroyImage(rhi->_vma, _image, _vmaAlloc);
	}
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
	if (!Texture::Init(desc))
		return false;
	auto rhi = static_pointer_cast<RhiVk>(_rhi.lock()); 
	auto queueFamilies = rhi->GetQueueFamilyIndices(_descriptor._usage);
	auto dimNatural = glm::max(_descriptor._dimensions, glm::uvec4(1));
	vk::ImageCreateInfo imgInfo{
		GetImageCreateFlags(_descriptor),
		GetImageType(_descriptor._dimensions),
		s_vk2Format.ToSrc(_descriptor._format, vk::Format::eUndefined),
		GetExtent3D(dimNatural),
		_descriptor._mipLevels,
		dimNatural[3],
		vk::SampleCountFlagBits::e1,
		_descriptor._usage.cpuAccess ? vk::ImageTiling::eLinear : vk::ImageTiling::eOptimal,
		GetImageUsage(_descriptor._usage, _descriptor._format),
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

bool TextureVk::Init(vk::Image image, ResourceDescriptor &desc, RhiOwned *owner)
{
	if (!Texture::Init(desc))
		return false;
	ASSERT(!_vmaAlloc);
	_image = image;
	_owner = owner->weak_from_this();
	return true;
}

bool TextureVk::InitView()
{
	auto rhi = static_pointer_cast<RhiVk>(_rhi.lock());
	vk::ImageViewCreateInfo viewInfo{
		vk::ImageViewCreateFlags(),
		_image,
		GetImageViewType(_descriptor),
		s_vk2Format.ToSrc(_descriptor._format, vk::Format::eUndefined),
		vk::ComponentMapping{vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity},
		vk::ImageSubresourceRange{
			GetImageAspect(_descriptor._format),
			0,
			_descriptor._mipLevels,
			0,
			_descriptor._dimensions[4]
		},
	};
	if (rhi->_device.createImageView(&viewInfo, rhi->AllocCallbacks(), &_view) != vk::Result::eSuccess)
		return false;

	return true;
}

}