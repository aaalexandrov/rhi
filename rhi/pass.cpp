#include "pass.h"
#include "rhi.h"

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

void PresentPass::EnumResources(ResourceEnum enumFn)
{
	ASSERT(bool(_swapchainTexture->_descriptor._usage & ResourceUsage{ .present=1 }));
	enumFn(_swapchainTexture.get(), ResourceUsage{.present=1, .read=1});
}

}