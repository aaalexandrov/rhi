#include "submit.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("submit", [] {
	TypeInfo::Register<Submission>().Name("Submission")
		.Base<RhiOwned>();
});


bool Submission::Init(std::vector<std::shared_ptr<Pass>> &&passes)
{
	_passes = std::move(passes);
	return true;
}

}