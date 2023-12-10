#pragma once

#include "base_vk.h"
#include "../resource.h"

namespace rhi {

struct PipelineVk : public Pipeline {


	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<PipelineVk>(); }

};

}