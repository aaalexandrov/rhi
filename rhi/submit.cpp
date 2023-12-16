#include "submit.h"
#include "pass.h"
#include "resource.h"
#include <deque>

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
	_passTransitions = ExtractResourceUse();

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

PassResourceTransitions Submission::ExtractResourceUse()
{
	struct ResourceUse {
		Resource *_resource = nullptr;
		Pass *_pass = nullptr;
		ResourceUsage _usage;
		ResourceUse *_prevUse = nullptr;
	};

	std::deque<ResourceUse> usedResources;
	std::unordered_map<Resource *, ResourceUse *> lastUses;
	PassResourceTransitions passTransitions;

	for (auto &pass : _passes) {
		pass->EnumResources([&](Resource *resource, ResourceUsage usage) {
			ResourceUse *&lastUse = lastUses[resource];
			usedResources.push_back(ResourceUse{
				._resource = resource,
				._pass = pass.get(),
				._usage = usage,
				._prevUse = lastUse
			});
			lastUse = &usedResources.back();
		});
	}

	for (auto [res, lastUse] : lastUses) {
		for (ResourceUse *use = lastUse; use; use = use->_prevUse) {
			ResourceUsage prevUsage = use->_prevUse ? use->_prevUse->_usage : use->_resource->_state;
			if (prevUsage == use->_usage)
				continue;
			passTransitions[use->_pass].push_back(ResourceTransition{
				._resource = use->_resource,
				._prevUsage = prevUsage,
				._usage = use->_usage,
			});
		}
		res->_state = lastUse->_usage;
	}

	return passTransitions;
}

}