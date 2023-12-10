#include "pass.h"
#include "rhi.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("pass", [] {
	TypeInfo::Register<Pass>().Name("Pass")
		.Base<RhiOwned>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<Rhi>());
	TypeInfo::Register<GraphicsPass>().Name("GraphicsPass")
		.Base<Pass>();
	TypeInfo::Register<ComputePass>().Name("ComputePass")
		.Base<Pass>();
	TypeInfo::Register<CopyPass>().Name("CopyPass")
		.Base<Pass>();
	TypeInfo::Register<PresentPass>().Name("PresentPass")
		.Base<Pass>();
});


}