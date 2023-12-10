#include "compute_pass_vk.h"
#include "rhi_vk.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("compute_pass_vk", [] {
	TypeInfo::Register<ComputePassVk>().Name("ComputePassVk")
		.Base<ComputePass>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());
});


}