#include "base.h"
#include "rhi.h"
#include <bit>

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("type_info", [] {
	TypeInfo::Register<WindowData>().Name("WindowData")
		.Base<utl::Any>();

	TypeInfo::Register<RhiOwned>().Name("RhiOwned")
		.Base<utl::Any>();

	TypeInfo::Register<Bindable>().Name("Bindable")
		.Base<RhiOwned>();
});


size_t Viewport::GetHash() const
{
	size_t hash = utl::GetHash(_rect);
	hash = utl::GetHash(_minDepth, hash);
	hash = utl::GetHash(_maxDepth, hash);
	return hash;
}

bool Viewport::operator==(Viewport const &other) const
{
	return _rect == other._rect && _minDepth == other._minDepth && _maxDepth == other._maxDepth;
}

size_t CullState::GetHash() const
{
	size_t hash = utl::GetHash(_front);
	hash = utl::GetHash(_cullMask, hash);
	return hash;
}

bool CullState::operator==(CullState const &other) const
{
	return _front == other._front && _cullMask == other._cullMask;
}

size_t DepthBias::GetHash() const
{
	size_t hash = utl::GetHash(_enable);
	hash = utl::GetHash(_constantFactor, hash);
	hash = utl::GetHash(_clamp, hash);
	hash = utl::GetHash(_slopeFactor, hash);
	return hash;
}

bool DepthBias::operator==(DepthBias const &other) const
{
	return _enable == other._enable
		&& _constantFactor == other._constantFactor
		&& _clamp == other._clamp
		&& _slopeFactor == other._slopeFactor;
}

size_t DepthState::GetHash() const
{
	size_t hash = utl::GetHash(_depthTestEnable);
	hash = utl::GetHash(_depthWriteEnable, hash);
	hash = utl::GetHash(_depthCompareFunc, hash);
	hash = utl::GetHash(_depthBoundsTestEnable, hash);
	hash = utl::GetHash(_minDepthBounds, hash);
	hash = utl::GetHash(_maxDepthBounds, hash);
	return hash;
}

bool DepthState::operator==(DepthState const &other) const
{
	return _depthTestEnable == other._depthTestEnable
		&& _depthWriteEnable == other._depthWriteEnable
		&& _depthCompareFunc == other._depthCompareFunc
		&& _depthBoundsTestEnable == other._depthBoundsTestEnable
		&& _minDepthBounds == other._minDepthBounds
		&& _maxDepthBounds == other._maxDepthBounds;
}

size_t StencilFuncState::GetHash() const
{
	size_t hash = utl::GetHash(_failFunc);
	hash = utl::GetHash(_passFunc, hash);
	hash = utl::GetHash(_depthFailFunc, hash);
	hash = utl::GetHash(_compareFunc, hash);
	hash = utl::GetHash(_compareMask, hash);
	hash = utl::GetHash(_writeMask, hash);
	hash = utl::GetHash(_reference, hash);
	return hash;
}

bool StencilFuncState::operator==(StencilFuncState const &other) const
{
	return _failFunc == other._failFunc
		&& _passFunc == other._passFunc
		&& _depthFailFunc == other._depthFailFunc
		&& _compareFunc == other._compareFunc
		&& _compareMask == other._compareMask
		&& _writeMask == other._writeMask
		&& _reference == other._reference;
}

size_t BlendFuncState::GetHash() const
{
	size_t hash = utl::GetHash(_blendEnable);
	hash = utl::GetHash(_srcColorBlendFactor, hash);
	hash = utl::GetHash(_dstColorBlendFactor, hash);
	hash = utl::GetHash(_colorBlendFunc, hash);
	hash = utl::GetHash(_srcAlphaBlendFactor, hash);
	hash = utl::GetHash(_dstAlphaBlendFactor, hash);
	hash = utl::GetHash(_alphaBlendFunc, hash);
	hash = utl::GetHash(_colorWriteMask, hash);
	return hash;
}

bool BlendFuncState::operator==(BlendFuncState const &other) const
{
	return _blendEnable == other._blendEnable
		&& _srcColorBlendFactor == other._srcColorBlendFactor
		&& _dstColorBlendFactor == other._dstColorBlendFactor
		&& _colorBlendFunc == other._colorBlendFunc
		&& _srcAlphaBlendFactor == other._srcAlphaBlendFactor
		&& _dstAlphaBlendFactor == other._dstAlphaBlendFactor
		&& _alphaBlendFunc == other._alphaBlendFunc
		&& _colorWriteMask == other._colorWriteMask;
}

size_t RenderState::GetHash() const
{
	size_t hash = _viewport.GetHash();
	hash = utl::GetHash(_scissor, hash);
	hash = 31 * hash + _cullState.GetHash();
	hash = 31 * hash + _depthBias.GetHash();
	hash = 31 * hash + _depthState.GetHash();
	hash = utl::GetHash(_stencilEnable, hash);
	hash = utl::GetHash(_stencilState, hash);
	hash = utl::GetHash(_blendStates, hash);
	hash = utl::GetHash(_blendColor, hash);
	return hash;
}

bool RenderState::operator==(RenderState const &other) const
{
	return _viewport == other._viewport
		&& _scissor == other._scissor
		&& _cullState == other._cullState
		&& _depthBias == other._depthBias
		&& _depthState == other._depthState
		&& _stencilEnable == other._stencilEnable
		&& _stencilState == other._stencilState
		&& _blendStates == other._blendStates
		&& _blendColor == other._blendColor;
}


glm::ivec4 ResourceDescriptor::GetNaturalDims(glm::ivec4 dims)
{
	return IsCube(dims) ? glm::ivec4(dims.x, dims.y, 0, dims.w) : dims;
}

bool ResourceDescriptor::IsCube(glm::ivec4 dims)
{
	// since it's invalid to have 3d texture arrays, we encode a cube map by placing 1 in the cube map's 3rd dimension
	return dims[0] > 0 && dims[1] > 0 && dims[2] == 1 && dims[3] > 0 && dims[3] % 6 == 0;
}


bool ResourceView::IsValidFor(ResourceDescriptor const &desc) const
{
	glm::bvec4 descEmptyDims = lessThanEqual(desc._dimensions, glm::ivec4(0));
	glm::bvec4 regionEmptyDims = lessThanEqual(_region.GetSize(), glm::ivec4(0));
	if (descEmptyDims != regionEmptyDims)
		return false;
	if (_mipRange.IsEmpty() != (desc._mipLevels <= 0))
		return false;

	return true;
}

ResourceView ResourceView::GetIntersection(ResourceView const &other) const
{
	ResourceView view{
		._format = _format,
		._region = _region.GetIntersection(other._region),
		._mipRange = _mipRange.GetIntersection(other._mipRange),
	};
	return view;
}

ResourceView ResourceView::GetIntersection(ResourceDescriptor &desc) const
{
	return GetIntersection(ResourceView::FromDescriptor(desc));
}

ResourceView ResourceView::FromDescriptor(ResourceDescriptor const &desc)
{
	ResourceView view{
		._format = desc._format,
		._region = utl::Box4I::FromMinAndSize(glm::ivec4(0), desc._dimensions),
		._mipRange = utl::IntervalI8::FromMinAndSize(0, desc._mipLevels),
	};
	return view;
}


RhiOwned::RhiOwned() 
{
}

RhiOwned::~RhiOwned() 
{
}

bool RhiOwned::InitRhi(Rhi *rhi, std::string name)
{
	ASSERT(!_rhi);
	ASSERT(_name.empty());
	_rhi = rhi;
	_name = name;
	return true;
}

bool ResourceRef::ValidateView()
{
	if (!_bindable)
		return true;

	Resource *resource = Cast<Resource>(_bindable.get());
	if (!resource)
		return true;
	if (_view._format == Format::Invalid)
		_view._format = resource->_descriptor._format;

	// TO DO: check view format compatibility

	_view = _view.GetIntersection(resource->_descriptor);

	return IsViewValid();
}

bool ResourceRef::IsViewValid()
{
	if (!_bindable)
		return false;
	Resource *resource = Cast<Resource>(_bindable.get());
	if (!resource)
		return true;
	return _view.IsValidFor(resource->_descriptor);
}

uint32_t GetFormatSize(Format fmt)
{
	TypeInfo const *type = s_format2TypeInfo[fmt];
	return type ? sizeof(type) : 0;
}

uint8_t GetMaxMipLevels(glm::ivec3 dims)
{
	uint32_t maxDim = utl::VecMaxElem(dims);
	auto numBits = std::bit_width(maxDim);
	ASSERT(maxDim > int(numBits == 1));
	return numBits;
}

glm::ivec3 GetMipLevelSize(glm::ivec3 dims, int32_t mipLevel)
{
	glm::ivec3 mipSize = dims >> mipLevel;
	if (all(lessThanEqual(mipSize, glm::ivec3(0))))
		return glm::ivec3(0);
	glm::ivec3 nonZero = glm::greaterThan(dims, glm::ivec3(0));
	mipSize = glm::max(mipSize, nonZero);
	return mipSize;
}

}
