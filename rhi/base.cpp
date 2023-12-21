#include "base.h"
#include "rhi.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("type_info", [] {
	TypeInfo::Register<WindowData>().Name("WindowData")
		.Base<utl::Any>();

	TypeInfo::Register<RhiOwned>().Name("RhiOwned")
		.Base<utl::Any>();
});


glm::ivec4 ResourceDescriptor::GetNaturalDims(glm::ivec4 dims)
{
	return IsCube(dims) ? glm::ivec4(dims.x, dims.y, 0, dims.w) : dims;
}

bool ResourceDescriptor::IsCube(glm::ivec4 dims)
{
	// since it's invalid to have 3d texture arrays, we encode a cube map by placing 1 in the cube map's 3rd dimension
	return dims[0] > 0 && dims[1] > 0 && dims[2] == 1 && dims[3] > 0 && dims[3] % 6 == 0;
}


bool ResourceView::IsEmpty() const
{
	return _region.IsEmpty() || _mipRange.IsEmpty();
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
	if (!_resource)
		return true;
	if (_view._format == Format::Invalid)
		_view._format = _resource->_descriptor._format;

	// TO DO: check view format compatibility

	_view = _view.GetIntersection(_resource->_descriptor);

	if (_view.IsEmpty())
		return false;

	return true;
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
