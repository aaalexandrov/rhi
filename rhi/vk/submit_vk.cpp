#include "submit_vk.h"
#include "rhi_vk.h"
#include "buffer_vk.h"
#include "texture_vk.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("submit_vk", [] {
	TypeInfo::Register<SubmissionVk>().Name("SubmissionVk")
		.Base<Submission>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());
});

void ExecuteDataVk::Clear()
{
	_fnExecute = nullptr;
	_waitSemaphores.clear();
	_cmds.clear();
	_signalSemaphores.clear();
}

bool ExecuteDataVk::CanCombine(ExecuteDataVk const &other)
{
	// we can combine a stage into the current data if all the later stages are empty
	if (other._fnExecute && (_fnExecute || _waitSemaphores.size() || _cmds.size() || _signalSemaphores.size()))
		return false;
	if (other._waitSemaphores.size() && (_cmds.size() || _signalSemaphores.size()))
		return false;
	if (other._cmds.size() && _signalSemaphores.size())
		return false;
	return true;
}

void ExecuteDataVk::Combine(ExecuteDataVk const &other)
{
	if (other._fnExecute) {
		ASSERT(!(_fnExecute || _waitSemaphores.size() || _cmds.size() || _signalSemaphores.size()));
		_fnExecute = other._fnExecute;
	}
	if (other._waitSemaphores.size()) {
		ASSERT(!(_cmds.size() || _signalSemaphores.size()));
		_waitSemaphores.insert(_waitSemaphores.end(), other._waitSemaphores.begin(), other._waitSemaphores.end());
	}
	if (other._cmds.size()) {
		ASSERT(!_signalSemaphores.size());
		_cmds.insert(_cmds.end(), other._cmds.begin(), other._cmds.end());
	}
	_signalSemaphores.insert(_signalSemaphores.end(), other._signalSemaphores.begin(), other._signalSemaphores.end());
}


bool SubmissionVk::InitRhi(Rhi *rhi, std::string name)
{
	if (!Submission::InitRhi(rhi, name))
		return false;
	auto rhiVk = static_pointer_cast<RhiVk>(_rhi.lock());
	if (!_recorder.Init(rhiVk.get(), rhiVk->_universalQueue._family))
		return false;

	return true;
}

bool SubmissionVk::Execute()
{
	ASSERT(!_executeSignalValue);
	// we're not calling the base execute, should it exist at all?
	for (auto &pass : _passes) {
		ExecuteDataVk execTransitions = RecordPassTransitionCmds(pass.get());
		if (!Execute(std::move(execTransitions)))
			return false;
		if (!pass->Execute(this))
			return false;
	}

	auto rhi = static_pointer_cast<RhiVk>(_rhi.lock());
	_executeSignalValue = ++rhi->_timelineSemaphore._value;

	ExecuteDataVk execSignalEnd;
	execSignalEnd._signalSemaphores.push_back(SemaphoreReferenceVk{
		._semaphore = rhi->_timelineSemaphore._semaphore,
		._counter = _executeSignalValue,
	});
	if (!Execute(std::move(execSignalEnd)))
		return false;
	FlushToExecute();

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

bool SubmissionVk::Execute(ExecuteDataVk &&execute)
{
	if (!_toExecute.CanCombine(execute)) {
		if (!FlushToExecute())
			return false;
	}
	_toExecute.Combine(execute);
	return true;
}

bool SubmissionVk::FlushToExecute()
{
	auto rhi = static_pointer_cast<RhiVk>(_rhi.lock());
	if (_toExecute._fnExecute) {
		if (!_toExecute._fnExecute(rhi->_universalQueue))
			return false;
	}

	std::vector<vk::Semaphore> waitSemaphores, signalSemaphores;
	std::vector<vk::PipelineStageFlags> waitStages;
	std::vector<uint64_t> waitSemValues, signalSemValues;
	std::vector<vk::CommandBuffer> cmds;
	for (auto &sem : _toExecute._waitSemaphores) {
		waitSemaphores.push_back(sem._semaphore);
		waitStages.push_back(sem._stages);
		waitSemValues.push_back(sem._counter);
	}
	for (auto &sem : _toExecute._signalSemaphores) {
		signalSemaphores.push_back(sem._semaphore);
		signalSemValues.push_back(sem._counter);
	}

	vk::TimelineSemaphoreSubmitInfo semValuesInfo{
		waitSemValues,
		signalSemValues,
	};
	vk::SubmitInfo submitInfo{
		waitSemaphores,
		waitStages,
		_toExecute._cmds,
		signalSemaphores, 
		&semValuesInfo
	};
	if (rhi->_universalQueue._queue.submit(submitInfo) != vk::Result::eSuccess)
		return false;

	_toExecute.Clear();

	return true;
}

ExecuteDataVk SubmissionVk::RecordPassTransitionCmds(Pass *pass)
{
	auto &transitions = _passTransitions[pass];

	auto rhi = static_pointer_cast<RhiVk>(_rhi.lock());
	ExecuteDataVk cmds;
	std::vector<vk::MemoryBarrier> memoryBarriers;
	std::vector<vk::BufferMemoryBarrier> bufferBarriers;
	std::deque<vk::ImageSubresourceRange> imageSubResRanges;
	std::vector<vk::ImageMemoryBarrier> imageBarriers;
	vk::PipelineStageFlags srcStages, dstStages;
	for (ResourceTransition &transition : transitions) {
		ASSERT(transition._prevUsage != transition._usage);

		auto *resourceVk = Cast<ResourceVk>(transition._resource);
		ResourceTransitionVk transitionData = resourceVk->GetTransitionData(transition._prevUsage, transition._usage);
		srcStages |= transitionData._srcState._stages;
		dstStages |= transitionData._dstState._stages;

		if (transitionData._srcState._semaphore._semaphore)
			cmds._waitSemaphores.push_back(transitionData._srcState._semaphore);
		if (transitionData._dstState._semaphore._semaphore)
			cmds._signalSemaphores.push_back(transitionData._dstState._semaphore);

		if (TextureVk *texture = Cast<TextureVk>(transition._resource)) {
			// image transition
			vk::ImageSubresourceRange subResRange{
				GetImageAspect(texture->_descriptor._format),
				0,
				texture->_descriptor._mipLevels,
				0,
				texture->_descriptor._dimensions[3]
			};
			imageSubResRanges.push_back(subResRange);
			vk::ImageMemoryBarrier imgBarrier{
				transitionData._srcState._access,
				transitionData._dstState._access,
				transitionData._srcState._layout,
				transitionData._dstState._layout,
				rhi->_universalQueue._family,
				rhi->_universalQueue._family,
				texture->_image,
				imageSubResRanges.back(),
			};
			imageBarriers.push_back(imgBarrier);
		} else if (BufferVk *buffer = Cast<BufferVk>(transition._resource)) {
			// buffer transition
			vk::BufferMemoryBarrier bufBarrier{
				transitionData._srcState._access,
				transitionData._dstState._access,
				rhi->_universalQueue._family,
				rhi->_universalQueue._family,
				buffer->_buffer,
				0,
				buffer->_descriptor._dimensions[0],
			};
			bufferBarriers.push_back(bufBarrier);
		}
	}

	ASSERT(!((cmds._waitSemaphores.size() || cmds._signalSemaphores.size()) && bufferBarriers.empty() && imageBarriers.empty() && memoryBarriers.empty()));
	if (bufferBarriers.empty() && imageBarriers.empty() && memoryBarriers.empty())
		return cmds;

	vk::CommandBuffer cmdBuf = _recorder.AllocCmdBuffer(vk::CommandBufferLevel::ePrimary, "X_" + pass->_name);
	vk::CommandBufferBeginInfo beginInfo{
		vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
	};
	vk::Result res = cmdBuf.begin(beginInfo);
	ASSERT(res == vk::Result::eSuccess);

	cmdBuf.pipelineBarrier(srcStages, dstStages, vk::DependencyFlags(), memoryBarriers, bufferBarriers, imageBarriers);

	res = cmdBuf.end();
	ASSERT(res == vk::Result::eSuccess);

	cmds._cmds.push_back(cmdBuf);

	return cmds;
}

}