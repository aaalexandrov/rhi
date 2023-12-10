#include "copy_pass_vk.h"
#include "rhi_vk.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("copy_pass_vk", [] {
	TypeInfo::Register<CopyPassVk>().Name("CopyPassVk")
		.Base<CopyPass>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());
});


}