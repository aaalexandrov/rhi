#include "copy_pass_vk.h"
#include "rhi_vk.h"
#include "buffer_vk.h"
#include "texture_vk.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("copy_pass_vk", [] {
	TypeInfo::Register<CopyPassVk>().Name("CopyPassVk")
		.Base<CopyPass>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());
});


bool CopyPassVk::InitRhi(Rhi *rhi, std::string name)
{
	if (!CopyPass::InitRhi(rhi, name))
		return false;

	auto rhiVk = static_cast<RhiVk *>(_rhi);
	if (!_recorder.Init(rhiVk, rhiVk->_universalQueue._family))
		return false;

	return true;
}

bool CopyPassVk::Prepare(Submission *sub)
{
	vk::CommandBuffer cmds = _recorder.BeginCmds(_name);
	if (!cmds)
		return false;

	for (auto &copy : _copies) {
		CopyType cpType = copy.GetCopyType();

		if (!cpType.srcTex && !cpType.dstTex) {
			CopyBufToBuf(copy);
		} else if (!cpType.srcTex && cpType.dstTex) {
			CopyBufToTex(copy);
		} else if (cpType.srcTex && !cpType.dstTex) {
			CopyTexToBuf(copy);
		} else {
			ASSERT(cpType.srcTex && cpType.dstTex);
			CopyTexToTex(copy);
		}
	}

	if (!_recorder.EndCmds(cmds))
		return false;

	return true;
}

bool CopyPassVk::Execute(Submission *sub)
{
	return _recorder.Execute(sub);
}

void CopyPassVk::CopyBufToBuf(CopyData &copy)
{
	vk::CommandBuffer cmds = _recorder._cmdBuffers.back();
	vk::BufferCopy region{
		(vk::DeviceSize)copy._src._view._region._min[0],
		(vk::DeviceSize)copy._dst._view._region._min[0],
		(vk::DeviceSize)copy._src._view._region.GetSize()[0],
	};
	ASSERT(copy._src._view._region.GetSize()[0] == copy._dst._view._region.GetSize()[0]);
	auto *srcBuf = static_cast<BufferVk *>(copy._src._bindable.get());
	auto *dstBuf = static_cast<BufferVk *>(copy._dst._bindable.get());

	cmds.copyBuffer(srcBuf->_buffer, dstBuf->_buffer, region);
}

void CopyPassVk::CopyTexToTex(CopyData &copy)
{
	vk::CommandBuffer cmds = _recorder._cmdBuffers.back();
	std::vector<vk::ImageCopy> regions;
	ASSERT(copy._src._view._region.GetSize() == copy._dst._view._region.GetSize());
	ASSERT(copy._src._view._mipRange.GetSize() == copy._dst._view._mipRange.GetSize());
	ASSERT(copy._src._view._mipRange.GetSize() > 0);
	for (uint32_t mipLevel = copy._src._view._mipRange._min; mipLevel <= copy._src._view._mipRange._max; ++mipLevel) {
		int32_t reduction = mipLevel - copy._src._view._mipRange._min;
		glm::ivec3 srcOffs = GetMipLevelSize(copy._src._view._region._min, reduction);
		glm::ivec3 dstOffs = GetMipLevelSize(copy._dst._view._region._min, reduction);
		glm::ivec3 size = GetMipLevelSize(copy._src._view._region.GetSize(), reduction);

		vk::ImageCopy region{
			GetImageSubresourceLayers(copy._src._view, mipLevel),
			GetOffset3D(srcOffs),
			GetImageSubresourceLayers(copy._dst._view, mipLevel),
			GetOffset3D(dstOffs),
			GetExtent3D(size),
		};
		regions.push_back(region);
	}
	auto *srcTex = static_cast<TextureVk *>(copy._src._bindable.get());
	auto *dstTex = static_cast<TextureVk *>(copy._dst._bindable.get());
	cmds.copyImage(srcTex->_image, vk::ImageLayout::eTransferSrcOptimal, dstTex->_image, vk::ImageLayout::eTransferDstOptimal, regions);
}

void CopyPassVk::CopyTexToBuf(CopyData &copy)
{
	ASSERT(copy._src._view._mipRange.GetSize() == 1);
	vk::CommandBuffer cmds = _recorder._cmdBuffers.back();
	uint32_t pixSize = GetFormatSize(copy._src._view._format);
	vk::BufferImageCopy region{
		(vk::DeviceSize)copy._dst._view._region._min[0],
		pixSize * copy._src._view._region.GetSize()[0],
		(uint32_t)copy._src._view._region.GetSize()[1],
		GetImageSubresourceLayers(copy._src._view),
		GetOffset3D(copy._src._view._region._min),
		GetExtent3D(copy._src._view._region.GetSize()),
	};
	auto *srcTex = static_cast<TextureVk *>(copy._src._bindable.get());
	auto *dstBuf = static_cast<BufferVk *>(copy._dst._bindable.get());
	cmds.copyImageToBuffer(srcTex->_image, vk::ImageLayout::eTransferSrcOptimal, dstBuf->_buffer, region);
}

void CopyPassVk::CopyBufToTex(CopyData &copy)
{
	ASSERT(copy._dst._view._mipRange.GetSize() == 1);
	vk::CommandBuffer cmds = _recorder._cmdBuffers.back();
	uint32_t pixSize = GetFormatSize(copy._dst._view._format);
	vk::BufferImageCopy region{
		(vk::DeviceSize)copy._src._view._region._min[0],
		pixSize * copy._dst._view._region.GetSize()[0],
		(uint32_t)copy._dst._view._region.GetSize()[1],
		GetImageSubresourceLayers(copy._dst._view),
		GetOffset3D(copy._dst._view._region._min),
		GetExtent3D(copy._dst._view._region.GetSize()),
	};
	auto *srcBuf = static_cast<BufferVk *>(copy._src._bindable.get());
	auto *dstTex = static_cast<TextureVk *>(copy._dst._bindable.get());
	cmds.copyBufferToImage(srcBuf->_buffer, dstTex->_image, vk::ImageLayout::eTransferDstOptimal, region);
}

}