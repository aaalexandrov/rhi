#include "submit_vk.h"
#include "rhi_vk.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("submit_vk", [] {
	TypeInfo::Register<SubmissionVk>().Name("SubmissionVk")
		.Base<Submission>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());
});


}