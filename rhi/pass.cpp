#include "pass.h"
#include "rhi.h"
#include "resource.h"
#include "pipeline.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("pass", [] {
	TypeInfo::Register<Pass>().Name("Pass")
		.Base<RhiOwned>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<Rhi>());
	TypeInfo::Register<GraphicsPass>().Name("GraphicsPass")
		.Base<Pass>();
	TypeInfo::Register<ComputePass>().Name("ComputePass")
		.Base<Pass>();
	TypeInfo::Register<CopyPass>().Name("CopyPass")
		.Base<Pass>();
	TypeInfo::Register<PresentPass>().Name("PresentPass")
		.Base<Pass>();
});


bool GraphicsPass::Init(std::span<TargetData> rts)
{
	_renderTargets.insert(_renderTargets.end(), rts.begin(), rts.end());
	return true;
}

void GraphicsPass::EnumResources(ResourceEnum enumFn)
{
	for (auto &target : _renderTargets) {
		ResourceUsage usage{ target._texture->_descriptor._usage & ResourceUsage{.rt=1, .ds=1} | ResourceUsage{.write=1} };
		ASSERT(bool(usage & ResourceUsage{ .rt = 1, .ds = 1 }));
		enumFn(target._texture.get(), usage);
	}
}

void PresentPass::SetSwapchainTexture(std::shared_ptr<Texture> tex)
{
	ASSERT(!_swapchainTexture);
	ASSERT(!_swapchain);
	_swapchainTexture = tex;
	_swapchain = GetSwapchain();
}

std::shared_ptr<Swapchain> PresentPass::GetSwapchain()
{
	if (!_swapchainTexture)
		return nullptr;
	return Cast<Swapchain>(_swapchainTexture->_owner.lock());
}

void PresentPass::EnumResources(ResourceEnum enumFn)
{
	ASSERT(bool(_swapchainTexture->_descriptor._usage & ResourceUsage{ .present=1 }));
	enumFn(_swapchainTexture.get(), ResourceUsage{.present=1, .read=1});
}

bool ComputePass::Init(Pipeline *pipeline, std::span<std::shared_ptr<ResourceSet>> resourceSets, glm::ivec3 numGroups)
{
	ASSERT(!_pipeline);
	ASSERT(_resourceSets.empty());

	if (!pipeline || any(lessThanEqual(pipeline->GetComputeGroupSize(), glm::ivec3(0))))
		return false;

	_pipeline = static_pointer_cast<Pipeline>(pipeline->shared_from_this());
	_resourceSets.insert(_resourceSets.end(), resourceSets.begin(), resourceSets.end());
	_numGroups = numGroups;


	// should we check resource sets are suitable for the pipeline? that numgroups are valid?

	return true;
}

void ComputePass::EnumResources(ResourceEnum enumFn)
{
	for (auto &set : _resourceSets) {
		set->EnumResources(enumFn);
	}
}

}