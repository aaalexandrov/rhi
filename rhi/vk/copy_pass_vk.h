#pragma once

#include "base_vk.h"
#include "../pass.h"

namespace rhi {

struct BufferVk;
struct TextureVk;
struct CopyPassVk final : CopyPass {
	bool NeedsMatchingTextures(CopyData &copy) override;

	bool Prepare(Submission *sub) override;
	bool Execute(Submission *sub) override;

	void CopyBufToBuf(CopyData &copy);
	void CopyTexToTex(CopyData &copy);
	void CopyTexToBuf(CopyData &copy);
	void CopyBufToTex(CopyData &copy);

	void BlitTexToTex(CopyData &copy);

	bool CanBlit(CopyData &copy);

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<CopyPassVk>(); }

	vk::CommandBuffer _cmds;
};

}