#include "graphics_pass_vk.h"
#include "rhi_vk.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("graphics_pass_vk", [] {
	TypeInfo::Register<GraphicsPassVk>().Name("GraphicsPassVk")
		.Base<GraphicsPass>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());
});


}