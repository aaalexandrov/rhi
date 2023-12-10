#pragma once

#include "base_vk.h"
#include "../pass.h"

namespace rhi {

struct PresentPassVk : public PresentPass {


	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<PresentPassVk>(); }

};

}