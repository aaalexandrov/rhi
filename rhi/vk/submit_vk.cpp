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
	auto rhiVk = static_cast<RhiVk*>(_rhi);
	if (!_recorder.Init(rhiVk, rhiVk->_universalQueue._family))
		return false;

	return true;
}

bool SubmissionVk::Execute()
{
	ASSERT(!_executeSignalValue);

	if (!Submission::Execute())
		return false;

	auto rhi = static_cast<RhiVk*>(_rhi);
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

bool SubmissionVk::ExecuteTransitions(Pass *pass)
{
	ExecuteDataVk execTransitions = RecordPassTransitionCmds(pass);
	if (!Execute(std::move(execTransitions)))
		return false;

	return true;
}

bool SubmissionVk::IsFinishedExecuting()
{
	if (!_executeSignalValue)
		return false;
	auto rhi = static_cast<RhiVk*>(_rhi);
	return _executeSignalValue <= rhi->_timelineSemaphore.GetCurrentCounter();
}

bool SubmissionVk::WaitUntilFinished()
{
	if (!_executeSignalValue)
		return false;
	
	auto rhi = static_cast<RhiVk*>(_rhi);
	bool res = rhi->_timelineSemaphore.WaitCounter(_executeSignalValue);
	uint64_t semCounter = rhi->_timelineSemaphore.GetCurrentCounter();
	ASSERT(semCounter <= _executeSignalValue);
	return res;
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

bool SubmissionVk::Execute(vk::CommandBuffer cmds)
{
	ExecuteDataVk exec;
	exec._cmds.push_back(cmds);
	if (!Execute(std::move(exec)))
		return false;
	return true;
}

bool SubmissionVk::FlushToExecute()
{
	auto rhi = static_cast<RhiVk*>(_rhi);
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

	auto rhi = static_cast<RhiVk*>(_rhi);
	ExecuteDataVk cmds;
	std::vector<vk::MemoryBarrier> memoryBarriers;
	std::vector<vk::BufferMemoryBarrier> bufferBarriers;
	std::vector<vk::ImageMemoryBarrier> imageBarriers;
	vk::PipelineStageFlags srcStages, dstStages;
	for (ResourceTransition &transition : transitions) {
		ASSERT(transition._prevUsage != transition._usage || transition._prevUsage.write && transition._usage.write);

		auto *resourceVk = Cast<ResourceVk>(transition._resource);
		ResourceTransitionVk transitionData = resourceVk->GetTransitionData(transition._prevUsage, transition._usage);
		srcStages |= transitionData._srcState._stages;
		dstStages |= transitionData._dstState._stages;

		if (transitionData._srcState._semaphore._semaphore) {
			transitionData._srcState._semaphore._stages = transitionData._dstState._stages;
			cmds._waitSemaphores.push_back(transitionData._srcState._semaphore);
		}
		if (transitionData._dstState._semaphore._semaphore)
			cmds._signalSemaphores.push_back(transitionData._dstState._semaphore);

		if (TextureVk *texture = Cast<TextureVk>(transition._resource)) {
			// image transition
			vk::ImageSubresourceRange subResRange{
				GetImageAspect(texture->_descriptor._format),
				0,
				(uint32_t)texture->_descriptor._mipLevels,
				0,
				std::max((uint32_t)texture->_descriptor._dimensions[3], 1u)
			};
			vk::ImageMemoryBarrier imgBarrier{
				transitionData._srcState._access,
				transitionData._dstState._access,
				transitionData._srcState._layout,
				transitionData._dstState._layout,
				rhi->_universalQueue._family,
				rhi->_universalQueue._family,
				texture->_image,
				subResRange,
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
				(uint32_t)buffer->_descriptor._dimensions[0],
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