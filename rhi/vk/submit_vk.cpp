#include "submit_vk.h"
#include "rhi_vk.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("submit_vk", [] {
	TypeInfo::Register<SubmissionVk>().Name("SubmissionVk")
		.Base<Submission>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());
});

bool SubmissionVk::Execute()
{
	if (!Submission::Execute())
		return false;

	auto rhi = static_pointer_cast<RhiVk>(_rhi.lock());
	_executeSignalValue = ++rhi->_timelineSemaphore._value;
	if (!FlushCommands(rhi.get(), ~0ull, _executeSignalValue))
		return false;

	return true;
}

bool SubmissionVk::IsFinishedExecuting()
{
	if (!_executeSignalValue)
		return false;
	auto rhi = static_pointer_cast<RhiVk>(_rhi.lock());
	return _executeSignalValue <= rhi->_timelineSemaphore.GetCurrentCounter();
}

bool SubmissionVk::WaitUntilFinished()
{
	if (!_executeSignalValue)
		return false;
	
	auto rhi = static_pointer_cast<RhiVk>(_rhi.lock());
	return rhi->_timelineSemaphore.WaitCounter(_executeSignalValue);
}

bool SubmissionVk::ExecuteDirect(DirectExecutionFunc fnExecute, vk::PipelineStageFlags dstStageFlags)
{
	auto rhi = static_pointer_cast<RhiVk>(_rhi.lock());
	if (!FlushCommands(rhi.get()))
		return false;
	if (!fnExecute(rhi->_universalQueue))
		return false;
	_toExecuteDstStageFlags |= dstStageFlags;

	return true;
}

bool SubmissionVk::ExecuteCommands(std::span<vk::CommandBuffer> cmdBuffers, vk::PipelineStageFlags dstStageFlags)
{
	_toExecute.insert(_toExecute.end(), cmdBuffers.begin(), cmdBuffers.end());
	_toExecuteDstStageFlags |= dstStageFlags;

	return true;
}

bool SubmissionVk::FlushCommands(RhiVk *rhi, uint64_t waitValue, uint64_t signalValue)
{
	std::vector<uint64_t> waitTimelineValues, signalTimelineValues;
	std::vector<vk::Semaphore> waitSemaphores, signalSemaphores;

	ASSERT(waitValue == ~0ull || signalValue != ~0ull || waitValue < signalValue);

	if (waitValue != ~0ull) {
		waitSemaphores.push_back(rhi->_timelineSemaphore._semaphore);
		waitTimelineValues.push_back(waitValue);
	}

	if (signalValue != ~0ull) {
		signalSemaphores.push_back(rhi->_timelineSemaphore._semaphore);
		signalTimelineValues.push_back(signalValue);
	}

	vk::TimelineSemaphoreSubmitInfo timelineInfo{
		waitTimelineValues,
		signalTimelineValues,
	};
	ASSERT(waitTimelineValues.size() == waitSemaphores.size());
	ASSERT(signalTimelineValues.size() == signalSemaphores.size());
	vk::SubmitInfo submitInfo{
		waitSemaphores,
		_toExecuteDstStageFlags,
		_toExecute,
		signalSemaphores,
		&timelineInfo
	};
	if (rhi->_universalQueue._queue.submit(submitInfo) != vk::Result::eSuccess)
		return false;

	_toExecute.clear();
	_toExecuteDstStageFlags = vk::PipelineStageFlags();

	return true;
}


}