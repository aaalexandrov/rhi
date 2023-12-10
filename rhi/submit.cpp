#include "submit.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("submit", [] {
	TypeInfo::Register<Submission>().Name("Submission")
		.Base<RhiOwned>();
});


}