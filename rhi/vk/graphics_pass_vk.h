#pragma once

#include "base_vk.h"
#include "../pass.h"

namespace rhi {

struct GraphicsPassVk : public GraphicsPass {
	~GraphicsPassVk() override;

	bool Init(std::span<TargetData> rts) override;

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