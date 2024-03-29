#include "graphics_pass_vk.h"
#include "rhi_vk.h"
#include "buffer_vk.h"
#include "texture_vk.h"
#include "submit_vk.h"
#include "pipeline_vk.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("graphics_pass_vk", [] {
	TypeInfo::Register<GraphicsPassVk>().Name("GraphicsPassVk")
		.Base<GraphicsPass>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());
});


GraphicsPassVk::~GraphicsPassVk()
{
	auto rhi = static_cast<RhiVk*>(_rhi);
	rhi->_device.destroyFramebuffer(_framebuffer, rhi->AllocCallbacks());
	rhi->_device.destroyRenderPass(_renderPass, rhi->AllocCallbacks());
}

bool GraphicsPassVk::InitRhi(Rhi *rhi, std::string name)
{
	if (!GraphicsPass::InitRhi(rhi, name))
		return false;

	auto rhiVk = static_cast<RhiVk *>(_rhi);
	if (!_recorder.Init(rhiVk, rhiVk->_universalQueue._family))
		return false;

	return true;
}

bool GraphicsPassVk::Init(std::span<RenderTargetData> rts, utl::BoxF const &viewport)
{
	if (!GraphicsPass::Init(rts, viewport))
		return false;

	if (!InitRenderPass())
		return false;

	if (!InitFramebuffer())
		return false;

	vk::CommandBuffer cmds = _recorder.BeginCmds(_name);
	if (!cmds)
		return false;

	std::vector<vk::ClearValue> clearValues;
	for (auto &rt : _renderTargets) {
		if (rt._texture->_descriptor._usage.ds) {
			clearValues.emplace_back(vk::ClearDepthStencilValue(rt._clearValue[0], (uint32_t)rt._clearValue[1]));
		} else {
			clearValues.emplace_back(vk::ClearColorValue(rt._clearValue[0], rt._clearValue[1], rt._clearValue[2], rt._clearValue[3]));
		}
	}
	vk::RenderPassBeginInfo passInfo{
		_renderPass,
		_framebuffer,
		vk::Rect2D(vk::Offset2D(0, 0), GetExtent2D(GetMinTargetSize())),
		clearValues,
	};
	cmds.beginRenderPass(passInfo, vk::SubpassContents::eInline);

	cmds.setViewport(0, GetViewport(_viewport));

	return true;
}

bool GraphicsPassVk::Draw(DrawData const &draw)
{
	if (!GraphicsPass::Draw(draw))
		return false;

	vk::CommandBuffer cmds = _recorder._cmdBuffers.back();

	auto *pipeVk = static_cast<PipelineVk *>(draw._pipeline.get());
	cmds.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeVk->_pipeline);

	std::vector<vk::DescriptorSet> descSets;
	for (auto &set : _resourceSets) {
		auto *setVk = static_cast<ResourceSetVk *>(set.get());
		utl::GetFromVec(descSets, setVk->_setIndex) = setVk->_descSet._set;
	}
	std::array<uint32_t, 0> noDynamicOffsets;
	cmds.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeVk->_layout, 0, descSets, noDynamicOffsets);

	if (pipeVk->_pipelineData._vertexInputs.size() != draw._vertexStreams.size())
		return false;

	std::vector<vk::Buffer> vertBuffers;
	std::vector<vk::DeviceSize> vertBufferOffsets;
	for (auto &vertStream : draw._vertexStreams) {
		auto *bufVk = static_cast<BufferVk *>(vertStream._buffer.get());
		vertBuffers.push_back(bufVk->_buffer);
		vertBufferOffsets.push_back(vertStream._offset);
	}
	cmds.bindVertexBuffers(0, vertBuffers, vertBufferOffsets);

	if (draw._indexStream._buffer) {
		auto *indBufVk = static_cast<BufferVk *>(draw._indexStream._buffer.get());
		ASSERT(indBufVk->_descriptor._format == Format::R16_u || indBufVk->_descriptor._format == Format::R32_u);
		cmds.bindIndexBuffer(indBufVk->_buffer, draw._indexStream._offset, indBufVk->_descriptor._format == Format::R16_u ? vk::IndexType::eUint16 : vk::IndexType::eUint32);

		cmds.drawIndexed(draw._indices.GetSize(), draw._instances.GetSize(), draw._indices._min, draw._vertexOffset, draw._instances._min);
	} else {
		cmds.draw(draw._indices.GetSize(), draw._instances.GetSize(), draw._indices._min, draw._instances._min);
	}

	return true;
}

bool GraphicsPassVk::InitRenderPass()
{
	auto rhi = static_cast<RhiVk *>(_rhi);
	ResourceUsage attachesUsage;
	std::vector<vk::AttachmentDescription> attachments;
	for (auto &rt : _renderTargets) {
		vk::AttachmentLoadOp loadOp = rt._clearValue[0] >= 0 ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
		vk::AttachmentStoreOp storeOp = vk::AttachmentStoreOp::eStore;
		ResourceUsage rtUsage = rt._texture->_descriptor._usage;
		vk::ImageLayout layout = GetImageLayout(rtUsage & ResourceUsage{ .rt = 1, .ds = 1 } | ResourceUsage{ .write = 1 });
		vk::AttachmentDescription attachDesc{
			vk::AttachmentDescriptionFlags(),
			s_vk2Format.ToSrc(rt._texture->_descriptor._format, vk::Format::eUndefined),
			vk::SampleCountFlagBits::e1,
			loadOp,
			storeOp,
			loadOp,
			storeOp,
			layout,
			layout,
		};
		attachments.push_back(attachDesc);
		attachesUsage |= rtUsage;
	}

	std::vector<vk::AttachmentReference> colorAttachRefs;
	vk::AttachmentReference depthAttachRef;
	std::array<vk::AttachmentReference, 0> noAttachRefs;
	for (uint32_t i = 0; i < attachments.size(); ++i) {
		vk::AttachmentReference &attachRef = _renderTargets[i]._texture->_descriptor._usage.ds
			? depthAttachRef
			: colorAttachRefs.emplace_back();
		attachRef.attachment = i;
		attachRef.layout = attachments[i].initialLayout;
	}

	bool hasDepthStencil = depthAttachRef.layout != vk::ImageLayout::eUndefined;
	std::array<vk::SubpassDescription, 1> subpasses{
		vk::SubpassDescription{}
			.setColorAttachments(colorAttachRefs)
			.setPDepthStencilAttachment(hasDepthStencil ? &depthAttachRef : nullptr),
	};

	vk::AccessFlags subpassAccess = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
	if (hasDepthStencil)
		subpassAccess |= vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
	std::array<vk::SubpassDependency, 2> subpassDependencies{
		vk::SubpassDependency{
			VK_SUBPASS_EXTERNAL,
			0,
			GetPipelineStages(attachesUsage),
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			GetAllAccess(attachesUsage),
			subpassAccess,
			vk::DependencyFlags()
		},
		vk::SubpassDependency{
			0, 
			VK_SUBPASS_EXTERNAL,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			subpassAccess,
			subpassAccess,
		},
	};
	vk::RenderPassCreateInfo passInfo{
		vk::RenderPassCreateFlags(),
		attachments,
		subpasses,
		subpassDependencies,
	};
	if (rhi->_device.createRenderPass(&passInfo, rhi->AllocCallbacks(), &_renderPass) != vk::Result::eSuccess)
		return false;

	return true;
}

bool GraphicsPassVk::InitFramebuffer()
{
	auto rhi = static_cast<RhiVk *>(_rhi);
	std::vector<vk::ImageView> attachViews;
	for (auto &rt : _renderTargets) {
		auto texVk = static_cast<TextureVk *>(rt._texture.get());
		attachViews.push_back(texVk->_view);
	}

	glm::uvec4 commonSize = GetMinTargetSize();
	vk::FramebufferCreateInfo frameInfo{
		vk::FramebufferCreateFlags(),
		_renderPass,
		attachViews,
		commonSize.x,
		commonSize.y,
		std::max(commonSize.w, 1u),
	};
	if (rhi->_device.createFramebuffer(&frameInfo, rhi->AllocCallbacks(), &_framebuffer) != vk::Result::eSuccess)
		return false;

	return true;
}

glm::ivec4 GraphicsPassVk::GetMinTargetSize()
{
	glm::ivec4 size{ std::numeric_limits<int32_t>::max() };
	for (auto &rt : _renderTargets)
		size = glm::min(size, rt._texture->_descriptor._dimensions);
	return size;
}


bool GraphicsPassVk::Prepare(Submission *sub)
{
	vk::CommandBuffer cmds = _recorder._cmdBuffers.back();

	cmds.endRenderPass();

	if (!_recorder.EndCmds(cmds))
		return false;

	return true;
}

bool GraphicsPassVk::Execute(Submission *sub)
{
	return _recorder.Execute(sub);
}

}