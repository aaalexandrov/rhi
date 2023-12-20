#include "compute_pass_vk.h"
#include "rhi_vk.h"
#include "submit_vk.h"
#include "pipeline_vk.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("compute_pass_vk", [] {
	TypeInfo::Register<ComputePassVk>().Name("ComputePassVk")
		.Base<ComputePass>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());
});


bool ComputePassVk::Prepare(Submission *sub)
{
	ASSERT(all(greaterThan(_pipeline->GetComputeGroupSize(), glm::ivec3(0))));

	vk::CommandBuffer cmds = _recorder.AllocCmdBuffer(vk::CommandBufferLevel::ePrimary, _name + std::to_string(_recorder._cmdBuffers.size()));
	vk::CommandBufferBeginInfo cmdBegin{
		vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
	};
	if (cmds.begin(cmdBegin) != vk::Result::eSuccess)
		return false;

	auto *pipeVk = static_cast<PipelineVk *>(_pipeline.get());
	cmds.bindPipeline(vk::PipelineBindPoint::eCompute, pipeVk->_pipeline);

	std::vector<vk::DescriptorSet> descSets;
	for (auto &set : _resourceSets) {
		auto *setVk = static_cast<ResourceSetVk *>(set.get());
		descSets.push_back(setVk->_descSet._set);
	}
	std::array<uint32_t, 0> noDynamicOffsets;
	cmds.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipeVk->_layout, 0, descSets, noDynamicOffsets);

	cmds.dispatch(_numGroups.x, _numGroups.y, _numGroups.z);

	if (cmds.end() != vk::Result::eSuccess)
		return false;

	return true;
}

bool ComputePassVk::Execute(Submission *sub)
{
	auto *subVk = static_cast<SubmissionVk *>(sub);

	ExecuteDataVk exec;
	exec._cmds.insert(exec._cmds.end(), _recorder._cmdBuffers.begin(), _recorder._cmdBuffers.end());
	if (!subVk->Execute(std::move(exec)))
		return false;

	return true;
}

}