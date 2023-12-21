#pragma once

#include "base_vk.h"
#include "../pass.h"

namespace rhi {

struct BufferVk;
struct TextureVk;
struct CopyPassVk : public CopyPass {
	bool Prepare(Submission *sub) override;
	bool Execute(Submission *sub) override;

	void CopyBufToBuf(CopyData &copy);
	void CopyTexToTex(CopyData &copy);
	void CopyTexToBuf(CopyData &copy);
	void CopyBufToTex(CopyData &copy);

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<CopyPassVk>(); }

	CmdRecorderVk _recorder;
};

}