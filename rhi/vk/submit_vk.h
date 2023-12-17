#pragma once

#include "base_vk.h"
#include "../submit.h"
#include "rhi_vk.h"

namespace rhi {

using DirectExecutionFuncVk = std::function<bool(RhiVk::QueueData &queue)>;
struct ExecuteDataVk {
	// executed in the order of the declarations below
	DirectExecutionFuncVk _fnExecute;
	std::vector<SemaphoreReferenceVk> _waitSemaphores;
	std::vector<vk::CommandBuffer> _cmds;
	std::vector<SemaphoreReferenceVk> _signalSemaphores;

	void Clear();
	bool CanCombine(ExecuteDataVk const &other);
	void Combine(ExecuteDataVk const &other);
};

struct SubmissionVk : public Submission {
	bool InitRhi(Rhi *rhi, std::string name) override;

	bool Execute() override;

	bool IsFinishedExecuting() override;
	bool WaitUntilFinished() override;

	bool Execute(ExecuteDataVk &&execute);
	bool FlushToExecute();

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<SubmissionVk>(); }

	ExecuteDataVk RecordPassTransitionCmds(Pass *pass);

	CmdRecorderVk _recorder;
	ExecuteDataVk _toExecute;
	uint64_t _executeSignalValue = 0;
};

}