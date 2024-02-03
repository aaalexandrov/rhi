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


bool GraphicsPass::Init(std::span<RenderTargetData> rts, utl::BoxF const &viewport)
{
	_renderTargets.insert(_renderTargets.end(), rts.begin(), rts.end());
	glm::ivec2 rtSize = _renderTargets[0]._texture->_descriptor._dimensions;
	utl::BoxF rtLimits = utl::BoxF::FromMinAndSize(glm::vec3(0), glm::vec3(rtSize, 1.0f));
	_viewport = rtLimits.GetIntersection(viewport);
	return true;
}

bool GraphicsPass::Draw(DrawData const &draw)
{
	_pipelines.insert(draw._pipeline);
	for (auto &set : draw._resourceSets) {
		_resourceSets.insert(set);
	}
	return true;
}

void GraphicsPass::EnumResources(ResourceEnum enumFn)
{
	for (auto &target : _renderTargets) {
		ResourceUsage usage{ target._texture->_descriptor._usage & ResourceUsage{.rt=1, .ds=1} | ResourceUsage{.write=1} };
		ASSERT(bool(usage & ResourceUsage{ .rt = 1, .ds = 1 }));
		enumFn(target._texture.get(), usage);
	}
	for (auto &set : _resourceSets) {
		set->EnumResources(enumFn);
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

	if (!pipeline || any(lessThanEqual(pipeline->_pipelineData.GetComputeGroupSize(), glm::ivec3(0))))
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

bool CopyPass::Copy(CopyData copy)
{
	Resource *srcRes = Cast<Resource>(copy._src._bindable.get());
	Resource *dstRes = Cast<Resource>(copy._dst._bindable.get());
	if (!srcRes || !dstRes)
		return false;
	if (!copy._src.ValidateView() || !copy._dst.ValidateView())
		return false;
	if (!srcRes->_descriptor._usage.copySrc || !dstRes->_descriptor._usage.copyDst) 
		return false;
	// Only allow multiple copies per pass if they have the same source / destination resources
	if (_copies.size() > 0 && (srcRes != _copies[0]._src._bindable.get() || dstRes != _copies[0]._dst._bindable.get()))
		return false;

	CopyType cpType = copy.GetCopyType();
	if (!cpType.srcTex && !cpType.dstTex || cpType.srcTex && cpType.dstTex && NeedsMatchingTextures(copy)) {
		glm::ivec4 regionSize = glm::min(copy._src._view._region.GetSize(), copy._dst._view._region.GetSize());
		copy._src._view._region.SetSize(regionSize);
		copy._dst._view._region.SetSize(regionSize);
	} else if (cpType.srcTex != cpType.dstTex) {
		// texture and buffer
		auto &refBuf = cpType.srcTex ? copy._dst : copy._src;
		auto &refTex = cpType.srcTex ? copy._src : copy._dst;
		uint32_t pixSize = GetFormatSize(refTex._view._format);
		uint32_t rowSize = pixSize * refTex._view._region.GetSize()[0];
		uint32_t sliceSize = rowSize * refTex._view._region.GetSize()[1];
		// 3d images can't be arrays, so we take the size of either the 3rd dimension, or array slices, whichever is greater
		glm::ivec4 texRgnSize = glm::max(refTex._view._region.GetSize(), glm::ivec4(1));
		uint32_t transferSize = sliceSize * std::max(texRgnSize[2], texRgnSize[3]);
		// not enough buffer space for all requested pixels?
		if (refBuf._view._region.GetSize()[0] < transferSize)
			return false;
	}

	if (cpType.srcTex && copy._src._view._mipRange.GetSize() != 1)
		return false;
	if (cpType.dstTex && copy._dst._view._mipRange.GetSize() != 1)
		return false;

	if (!copy._src.IsViewValid() || !copy._dst.IsViewValid())
		return false;

	if (copy._src._bindable == copy._dst._bindable) {
		utl::IntervalI srcArrRange = copy._src._view.GetArrayRange();
		utl::IntervalI dstArrRange = copy._dst._view.GetArrayRange();
		if (srcArrRange.Intersects(dstArrRange) && copy._src._view._mipRange.Intersects(copy._dst._view._mipRange)) {
			ASSERT(0);
			return false;
		}
	}

	_copies.push_back(std::move(copy));

	return true;
}

bool CopyPass::CopyTopToLowerMips(std::shared_ptr<Texture> tex)
{
	if (_name.empty())
		_name = tex->_name + "MipGen";

	for (int8_t mip = 1; mip < tex->_descriptor._mipLevels; ++mip) {
		bool res = Copy({ 
			{tex, ResourceView::FromDescriptor(tex->_descriptor, mip - 1, 1)}, 
			{tex, ResourceView::FromDescriptor(tex->_descriptor, mip - 0, 1)} 
		});
		if (!res)
			return false;
	}

	return true;
}

bool CopyPass::CopyMips(std::shared_ptr<Texture> src, std::shared_ptr<Texture> dst, int8_t srcMip, int8_t dstMip, int8_t numMips)
{
	if (_name.empty())
		_name = src->_name + "->" + dst->_name;

	auto srcView = ResourceView::FromDescriptor(src->_descriptor, srcMip);
	auto dstView = ResourceView::FromDescriptor(dst->_descriptor, dstMip);
	if (srcView._region != dstView._region)
		return false;

	numMips = std::min(numMips, std::min(srcView._mipRange.GetSize(), dstView._mipRange.GetSize()));
	for (int8_t m = 0; m < numMips; ++m) {
		bool res = Copy({
			{src, ResourceView::FromDescriptor(src->_descriptor, srcMip + m, 1)},
			{dst, ResourceView::FromDescriptor(dst->_descriptor, dstMip + m, 1)}
		});
		if (!res)
			return false;
	}

	return true;
}

void CopyPass::EnumResources(ResourceEnum enumFn)
{
	for (auto &copy : _copies) {
		enumFn(static_cast<Resource *>(copy._src._bindable.get()), ResourceUsage{ .copySrc = 1, .read = 1  });
		if (copy._dst._bindable != copy._src._bindable)
			enumFn(static_cast<Resource *>(copy._dst._bindable.get()), ResourceUsage{ .copyDst = 1, .write = 1 });
	}
}

auto CopyPass::CopyData::GetCopyType() const -> CopyType
{
	CopyType cpType{
		.srcTex = Cast<Texture>(_src._bindable.get()) != nullptr,
		.dstTex = Cast<Texture>(_dst._bindable.get()) != nullptr,
	};
	ASSERT(cpType.srcTex == (Cast<Buffer>(_src._bindable.get()) == nullptr));
	ASSERT(cpType.dstTex == (Cast<Buffer>(_dst._bindable.get()) == nullptr));

	return cpType;
}

}
