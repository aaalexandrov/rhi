#pragma once

#include "base_vk.h"
#include "../pass.h"

namespace rhi {

struct ComputePassVk final : ComputePass {
	bool Prepare(Submission *sub) override;
	bool Execute(Submission *sub) override;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<ComputePassVk>(); }

	vk::CommandBuffer _cmds;
};

}