#include "texture_vk.h"
#include "rhi_vk.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("texture_vk", [] {
	TypeInfo::Register<TextureVk>().Name("TextureVk")
		.Base<Texture>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());
});


}