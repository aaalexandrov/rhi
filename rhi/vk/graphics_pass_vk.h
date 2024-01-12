#pragma once

#include "base_vk.h"
#include "../pass.h"

namespace rhi {

struct GraphicsPassVk final : GraphicsPass {
	~GraphicsPassVk() override;

	bool InitRhi(Rhi *rhi, std::string name) override;
	bool Init(std::span<TargetData> rts, utl::BoxF const &viewport = utl::BoxF::GetMaximum()) override;

	bool Draw(DrawData const &draw) override;

	bool Prepare(Submission *sub) override;
	bool Execute(Submission *sub) override;

	bool InitRenderPass();
	bool InitFramebuffer();

	glm::ivec4 GetMinTargetSize();

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<GraphicsPassVk>(); }

	vk::RenderPass _renderPass;
	vk::Framebuffer _framebuffer;
	CmdRecorderVk _recorder;
};

}