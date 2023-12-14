#pragma once

#include "base.h"

namespace rhi {

struct Pass;
struct Submission : public RhiOwned {
	virtual bool Init(std::vector<std::shared_ptr<Pass>> &&passes);

	virtual bool Prepare() = 0;
	virtual bool Execute() = 0;

	virtual bool IsFinishedExecuting() = 0;
	virtual bool WaitUntilFinished() = 0;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Submission>(); }

	std::vector<std::shared_ptr<Pass>> _passes;
};

}