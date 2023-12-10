#pragma once

#include "base_vk.h"
#include "../pass.h"

namespace rhi {

struct CopyPassVk : public CopyPass {


	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<CopyPassVk>(); }

};

}