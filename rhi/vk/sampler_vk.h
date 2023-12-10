#pragma once

#include "base_vk.h"
#include "../resource.h"

namespace rhi {

struct SamplerVk : public Sampler {


	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<SamplerVk>(); }

};

}