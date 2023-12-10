#include "buffer_vk.h"
#include "rhi_vk.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("buffer_vk", [] {
	TypeInfo::Register<BufferVk>().Name("BufferVk")
		.Base<Buffer>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());
});


}