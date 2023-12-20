#pragma once

#include "base_vk.h"
#include "../pass.h"

namespace rhi {

struct CopyPassVk : public CopyPass {
	bool Prepare(Submission *sub) override;
	bool Execute(Submission *sub) override;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<CopyPassVk>(); }

	CmdRecorderVk _recorder;
};

}