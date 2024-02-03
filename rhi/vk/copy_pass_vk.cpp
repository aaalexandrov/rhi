#include "copy_pass_vk.h"
#include "rhi_vk.h"
#include "buffer_vk.h"
#include "texture_vk.h"
#include "submit_vk.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("copy_pass_vk", [] {
	TypeInfo::Register<CopyPassVk>().Name("CopyPassVk")
		.Base<CopyPass>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());
});


bool CopyPassVk::NeedsMatchingTextures(CopyData &copy)
{
	return !CanBlit(copy);
}

void RecordTransferBarrier(vk::CommandBuffer cmds, uint32_t queueFamily, ResourceRef &ref, bool toDst)
{
	vk::AccessFlags srcAccess = toDst ? vk::AccessFlagBits::eTransferRead : vk::AccessFlagBits::eTransferWrite;
	vk::AccessFlags dstAccess = toDst ? vk::AccessFlagBits::eTransferWrite : vk::AccessFlagBits::eTransferRead;
	if (auto *texVk = Cast<TextureVk>(ref._bindable.get())) {
		vk::ImageMemoryBarrier imgBarrier{
			srcAccess,
			dstAccess,
			toDst ? vk::ImageLayout::eTransferSrcOptimal : vk::ImageLayout::eTransferDstOptimal,
			toDst ? vk::ImageLayout::eTransferDstOptimal : vk::ImageLayout::eTransferSrcOptimal,
			queueFamily,
			queueFamily,
			texVk->_image,
			GetViewSubresourceRange(ref._view),
		};
		cmds.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eTransfer,
			vk::DependencyFlags(),
			nullptr,
			nullptr,
			imgBarrier);
	} else if (auto *bufVk = Cast<BufferVk>(ref._bindable.get())) {
		vk::BufferMemoryBarrier bufBarrier{
			srcAccess,
			dstAccess,
			queueFamily,
			queueFamily,
			bufVk->_buffer,
			(vk::DeviceSize)ref._view._region._min[0],
			(vk::DeviceSize)ref._view._region.GetSize()[0],
		};
		cmds.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eTransfer,
			vk::DependencyFlags(),
			nullptr,
			bufBarrier,
			nullptr);

	} else {
		ASSERT(0);
	}
}

bool CopyPassVk::Prepare(Submission *sub)
{
	auto subVk = static_cast<SubmissionVk *>(sub);
	_cmds = subVk->_recorder.BeginCmds(_name);
	if (!_cmds)
		return false;

	for (auto &copy : _copies) {
		CopyType cpType = copy.GetCopyType();

		if (copy._src._bindable == copy._dst._bindable) 
			RecordTransferBarrier(_cmds, subVk->_recorder._queueFamily, copy._dst, true);

		if (!cpType.srcTex && !cpType.dstTex) {
			CopyBufToBuf(copy);
		} else if (!cpType.srcTex && cpType.dstTex) {
			CopyBufToTex(copy);
		} else if (cpType.srcTex && !cpType.dstTex) {
			CopyTexToBuf(copy);
		} else {
			ASSERT(cpType.srcTex && cpType.dstTex);

			if (CanBlit(copy))
				BlitTexToTex(copy);
			else
				CopyTexToTex(copy);
		}

		if (copy._src._bindable == copy._dst._bindable)
			RecordTransferBarrier(_cmds, subVk->_recorder._queueFamily, copy._dst, false);
	}

	if (!subVk->_recorder.EndCmds(_cmds))
		return false;

	return true;
}

bool CopyPassVk::Execute(Submission *sub)
{
	auto *subVk = static_cast<SubmissionVk *>(sub);
	return subVk->Execute(_cmds);
}

void CopyPassVk::CopyBufToBuf(CopyData &copy)
{
	vk::BufferCopy region{
		(vk::DeviceSize)copy._src._view._region._min[0],
		(vk::DeviceSize)copy._dst._view._region._min[0],
		(vk::DeviceSize)copy._src._view._region.GetSize()[0],
	};
	ASSERT(copy._src._view._region.GetSize()[0] == copy._dst._view._region.GetSize()[0]);
	auto *srcBuf = static_cast<BufferVk *>(copy._src._bindable.get());
	auto *dstBuf = static_cast<BufferVk *>(copy._dst._bindable.get());

	_cmds.copyBuffer(srcBuf->_buffer, dstBuf->_buffer, region);
}

void CopyPassVk::CopyTexToTex(CopyData &copy)
{
	ASSERT(copy._src._view._region.GetSize() == copy._dst._view._region.GetSize());
	ASSERT(copy._src._view._mipRange.GetSize() == copy._dst._view._mipRange.GetSize());
	ASSERT(copy._src._view._mipRange.GetSize() == 1);

	vk::ImageCopy region{
		GetImageSubresourceLayers(copy._src._view),
		GetOffset3D(copy._src._view._region._min),
		GetImageSubresourceLayers(copy._dst._view),
		GetOffset3D(copy._dst._view._region._min),
		GetExtent3D(copy._src._view._region.GetSize()),
	};
	auto *srcTex = static_cast<TextureVk *>(copy._src._bindable.get());
	auto *dstTex = static_cast<TextureVk *>(copy._dst._bindable.get());
	_cmds.copyImage(srcTex->_image, vk::ImageLayout::eTransferSrcOptimal, dstTex->_image, vk::ImageLayout::eTransferDstOptimal, region);
}

void CopyPassVk::BlitTexToTex(CopyData &copy)
{
	ASSERT(copy._src._view._mipRange.GetSize() == copy._dst._view._mipRange.GetSize());
	ASSERT(copy._src._view._mipRange.GetSize() == 1);

	glm::ivec3 srcOffs = copy._src._view._region._min;
	glm::ivec3 srcSize = max(glm::ivec3(copy._src._view._region.GetSize()), glm::ivec3(1));
	glm::ivec3 dstOffs = copy._dst._view._region._min;
	glm::ivec3 dstSize = max(glm::ivec3(copy._dst._view._region.GetSize()), glm::ivec3(1));

	vk::ImageBlit region{
		GetImageSubresourceLayers(copy._src._view),
		{GetOffset3D(srcOffs), GetOffset3D(srcOffs + srcSize)},
		GetImageSubresourceLayers(copy._dst._view),
		{GetOffset3D(dstOffs), GetOffset3D(dstOffs + dstSize)},
	};
	auto *srcTex = static_cast<TextureVk *>(copy._src._bindable.get());
	auto *dstTex = static_cast<TextureVk *>(copy._dst._bindable.get());
	_cmds.blitImage(srcTex->_image, vk::ImageLayout::eTransferSrcOptimal, dstTex->_image, vk::ImageLayout::eTransferDstOptimal, region, vk::Filter::eLinear);
}

bool CopyPassVk::CanBlit(CopyData &copy)
{
	auto rhi = static_cast<RhiVk *>(_rhi);
	auto *srcTex = static_cast<TextureVk *>(copy._src._bindable.get());
	auto *dstTex = static_cast<TextureVk *>(copy._dst._bindable.get());
	vk::FormatFeatureFlags srcFeatures = rhi->GetFormatFeatures(srcTex->_descriptor._format, srcTex->_descriptor._usage);
	vk::FormatFeatureFlags dstFeatures = rhi->GetFormatFeatures(dstTex->_descriptor._format, dstTex->_descriptor._usage);

	return (srcFeatures & vk::FormatFeatureFlagBits::eBlitSrc) && (dstFeatures & vk::FormatFeatureFlagBits::eBlitDst);
}

void CopyPassVk::CopyTexToBuf(CopyData &copy)
{
	ASSERT(copy._src._view._mipRange.GetSize() == 1);
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
	_cmds.copyImageToBuffer(srcTex->_image, vk::ImageLayout::eTransferSrcOptimal, dstBuf->_buffer, region);
}

void CopyPassVk::CopyBufToTex(CopyData &copy)
{
	ASSERT(copy._dst._view._mipRange.GetSize() == 1);
	vk::BufferImageCopy region{
		(vk::DeviceSize)copy._src._view._region._min[0],
		(uint32_t)copy._dst._view._region.GetSize()[0],
		(uint32_t)copy._dst._view._region.GetSize()[1],
		GetImageSubresourceLayers(copy._dst._view),
		GetOffset3D(copy._dst._view._region._min),
		GetExtent3D(copy._dst._view._region.GetSize()),
	};
	auto *srcBuf = static_cast<BufferVk *>(copy._src._bindable.get());
	auto *dstTex = static_cast<TextureVk *>(copy._dst._bindable.get());
	_cmds.copyBufferToImage(srcBuf->_buffer, dstTex->_image, vk::ImageLayout::eTransferDstOptimal, region);
}

}