#include "sampler_vk.h"
#include "rhi_vk.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("sampler_vk", [] {
	TypeInfo::Register<SamplerVk>().Name("SamplerVk")
		.Base<Sampler>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());
});


}