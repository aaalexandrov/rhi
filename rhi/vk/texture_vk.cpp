#include "texture_vk.h"
#include "rhi_vk.h"
#include "swapchain_vk.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("texture_vk", [] {
	TypeInfo::Register<TextureVk>().Name("TextureVk")
		.Base<Texture>()
		.Base<ResourceVk>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());
});


vk::ImageCreateFlags GetImageCreateFlags(ResourceDescriptor const &desc)
{
	vk::ImageCreateFlags flags = {};
	if (desc.IsCube())
		flags |= vk::ImageCreateFlagBits::eCubeCompatible;
	if (GetImageType(desc._dimensions) == vk::ImageType::e3D)
		flags |= vk::ImageCreateFlagBits::e2DArrayCompatible;
	return flags;
}

vk::ImageType GetImageType(glm::ivec4 dims)
{
	dims = ResourceDescriptor::GetNaturalDims(dims);
	ASSERT(dims.x > 0);
	if (dims.y == 0)
		return vk::ImageType::e1D;
	if (dims.z == 0)
		return vk::ImageType::e2D;
	return vk::ImageType::e3D;
}

vk::ImageViewType GetImageViewType(glm::ivec4 dims)
{
	bool isCube = ResourceDescriptor::IsCube(dims);
	dims = ResourceDescriptor::GetNaturalDims(dims);
	auto imgType = GetImageType(dims);
	bool isArray = dims[3] > 0;
	switch (imgType) {
		case vk::ImageType::e1D:
			return isArray ? vk::ImageViewType::e1DArray : vk::ImageViewType::e1D;
		case vk::ImageType::e2D:
			if (isCube)
				return dims[3] > 6 ? vk::ImageViewType::eCubeArray : vk::ImageViewType::eCube;
			return isArray ? vk::ImageViewType::e2DArray : vk::ImageViewType::e2D;
		default:
			ASSERT(imgType == vk::ImageType::e3D);
			return vk::ImageViewType::e3D;
	}
}

vk::ImageUsageFlags GetImageUsage(ResourceUsage usage, Format imgFormat)
{
	vk::ImageUsageFlags imgUsage = (vk::ImageUsageFlags)s_vkImageUsage2ResourceUsage.ToSrc(usage);
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
	auto rhi = static_cast<RhiVk*>(_rhi);
	rhi->_device.destroyImageView(_view, rhi->AllocCallbacks());
	if (_vmaAlloc) {
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
	auto rhi = static_cast<RhiVk*>(_rhi); 
	auto queueFamilies = rhi->GetQueueFamilyIndices(_descriptor._usage);
	auto dimNatural = glm::max(_descriptor._dimensions, glm::ivec4(1));
	vk::ImageCreateInfo imgInfo{
		GetImageCreateFlags(_descriptor),
		GetImageType(_descriptor._dimensions),
		s_vk2Format.ToSrc(_descriptor._format, vk::Format::eUndefined),
		GetExtent3D(dimNatural),
		(uint32_t)_descriptor._mipLevels,
		(uint32_t)dimNatural[3],
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
	_view = CreateView(ResourceView::FromDescriptor(_descriptor));
	if (!_view)
		return false;
	return true;
}

ResourceTransitionVk TextureVk::GetTransitionData(ResourceUsage prevUsage, ResourceUsage usage)
{
	ResourceTransitionVk data{
		._srcState = GetState(prevUsage),
		._dstState = GetState(usage),
	};
	return data;
}

ResourceStateVk TextureVk::GetState(ResourceUsage usage)
{
	ResourceStateVk state;
	state._access = GetAccess(usage);
	state._stages = GetPipelineStages(usage);
	state._layout = usage.create ? GetInitialLayout() : GetImageLayout(usage);

	if (usage.present && usage.write) {
		// present acquired, need to wait for the image's swapchain semaphore
		auto swapchain = Cast<SwapchainVk>(_owner.lock());
		int32_t texIndex = swapchain->GetTextureIndex(this);
		ASSERT(0 <= texIndex && texIndex < swapchain->_images.size());
		state._semaphore = SemaphoreReferenceVk{
			._semaphore = swapchain->_acquireSemaphores[texIndex],
		};
	}

	return state;
}

vk::ImageView TextureVk::CreateView(ResourceView const &view)
{
	glm::ivec4 viewDims = view._region.GetSize();
	auto rhi = static_cast<RhiVk *>(_rhi);
	vk::ImageViewCreateInfo viewInfo{
		vk::ImageViewCreateFlags(),
		_image,
		GetImageViewType(viewDims),
		s_vk2Format.ToSrc(view._format, vk::Format::eUndefined),
		vk::ComponentMapping{vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity},
		vk::ImageSubresourceRange{
			GetImageAspect(view._format),
			(uint32_t)view._mipRange._min,
			(uint32_t)view._mipRange.GetSize(),
			(uint32_t)view._region._min[3],
			(uint32_t)std::max(viewDims[3], 1)
		},
	};
	vk::ImageView imgView;
	if (rhi->_device.createImageView(&viewInfo, rhi->AllocCallbacks(), &imgView) != vk::Result::eSuccess)
		return vk::ImageView();


	return imgView;
}

}