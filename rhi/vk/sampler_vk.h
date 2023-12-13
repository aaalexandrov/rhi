#pragma once

#include "base_vk.h"
#include "../resource.h"

namespace rhi {

struct SamplerVk : public Sampler {
	~SamplerVk() override;

	bool Init(SamplerDescriptor const &desc) override;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<SamplerVk>(); }

	vk::Sampler _sampler;
};

}