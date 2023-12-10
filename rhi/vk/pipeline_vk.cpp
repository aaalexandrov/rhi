#include "pipeline_vk.h"
#include "rhi_vk.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("pipeline_vk", [] {
	TypeInfo::Register<PipelineVk>().Name("PipelineVk")
		.Base<Pipeline>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());
});


}