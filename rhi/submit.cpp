#include "submit.h"
#include "pass.h"

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

bool Submission::Prepare()
{
	for (auto &pass : _passes) {
		if (!pass->Prepare(this))
			return false;
	}
	return true;
}

bool Submission::Execute()
{
	for (auto &pass : _passes) {
		if (!pass->Execute(this))
			return false;
	}
	return true;
}

}