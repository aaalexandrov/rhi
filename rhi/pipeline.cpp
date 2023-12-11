#include "pipeline.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("resource", [] {
	TypeInfo::Register<Pipeline>().Name("Pipeline")
		.Base<RhiOwned>();
});

}