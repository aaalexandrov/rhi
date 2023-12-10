#pragma once

#include "base_vk.h"
#include "../pass.h"

namespace rhi {

struct ComputePassVk : public ComputePass {


	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<ComputePassVk>(); }

};

}