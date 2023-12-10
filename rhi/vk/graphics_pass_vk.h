#pragma once

#include "base_vk.h"
#include "../pass.h"

namespace rhi {

struct GraphicsPassVk : public GraphicsPass {


	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<GraphicsPassVk>(); }

};

}