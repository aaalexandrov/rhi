#include "present_pass_vk.h"
#include "rhi_vk.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("present_pass_vk", [] {
	TypeInfo::Register<PresentPassVk>().Name("PresentPassVk")
		.Base<PresentPass>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());
});


}