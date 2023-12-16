#pragma once

#include "base.h"

namespace rhi {

struct Pass;
struct Resource;

struct ResourceTransition {
	Resource *_resource = nullptr;
	ResourceUsage _prevUsage, _usage;
};
using PassResourceTransitions = std::unordered_map<Pass *, std::vector<ResourceTransition>>;

struct Submission : public RhiOwned {
	virtual bool Init(std::vector<std::shared_ptr<Pass>> &&passes);

	virtual bool Prepare();
	virtual bool Execute();

	virtual bool IsFinishedExecuting() = 0;
	virtual bool WaitUntilFinished() = 0;

	PassResourceTransitions ExtractResourceUse();

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Submission>(); }

	std::vector<std::shared_ptr<Pass>> _passes;
	PassResourceTransitions _passTransitions;
};

}