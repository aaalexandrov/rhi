#pragma once

#include "base_vk.h"
#include "../pass.h"

namespace rhi {

struct PresentPassVk final : PresentPass {

	bool Prepare(Submission *sub) override;
	bool Execute(Submission *sub) override;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<PresentPassVk>(); }

	vk::Result _presentResult;
};

}