#pragma once

#include "base_vk.h"
#include "../submit.h"
#include "rhi_vk.h"

namespace rhi {

struct SubmissionVk : public Submission {

	bool Prepare() override;
	bool Execute() override;

	bool IsFinishedExecuting() override;
	bool WaitUntilFinished() override;

	using DirectExecutionFunc = std::function<bool(RhiVk::QueueData &queue)>;
	bool ExecuteDirect(DirectExecutionFunc fnExecute, vk::PipelineStageFlags dstStageFlags);
	bool ExecuteCommands(std::span<vk::CommandBuffer> cmdBuffers, vk::PipelineStageFlags dstStageFlags);

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<SubmissionVk>(); }

	bool FlushCommands(RhiVk *rhi, uint64_t waitValue = ~0ull, uint64_t signalValue = ~0ull);

	vk::CommandBuffer RecordPassTransitionCmds(Pass *pass);

	CmdRecorderVk _recorder;
	std::vector<vk::CommandBuffer> _perPassTransitionCmds;
	std::vector<vk::CommandBuffer> _toExecute;
	vk::PipelineStageFlags _toExecuteDstStageFlags;
	uint64_t _executeSignalValue = 0;
};

}