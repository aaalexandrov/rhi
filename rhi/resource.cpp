#include "resource.h"
#include "rhi.h"
#include "utl/mathutl.h"
#include <bit>


namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("resource", [] {
	TypeInfo::Register<Resource>().Name("Resource")
		.Base<RhiOwned>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<Rhi>());
	TypeInfo::Register<Buffer>().Name("Buffer")
		.Base<Resource>();
	TypeInfo::Register<Texture>().Name("Texture")
		.Base<Resource>();
	TypeInfo::Register<Sampler>().Name("Sampler")
		.Base<RhiOwned>();
	TypeInfo::Register<Swapchain>().Name("Swapchain")
		.Base<RhiOwned>();
});


uint8_t ResourceDescriptor::GetMaxMipLevels(glm::uvec3 dims)
{
	uint32_t maxDim = utl::VecMaxElem(dims);
	auto numBits = std::bit_width(maxDim);
	ASSERT(maxDim > numBits == 1);
	return numBits;
}

bool Resource::Init(ResourceDescriptor const &desc)
{
	_descriptor = desc;
	return true;
}

bool Texture::Init(ResourceDescriptor const &desc)
{
	if (!Resource::Init(desc))
		return false;
	ASSERT(_descriptor._dimensions[0] > 0);
	ASSERT(_descriptor._dimensions[1] == 0);
	ASSERT(_descriptor._dimensions[2] == 0);
	ASSERT(_descriptor._dimensions[3] == 0);

	return true;
}

bool Sampler::Init(SamplerDescriptor const &desc)
{
	_descriptor = desc;
	return true;
}

bool Swapchain::Init(SwapchainDescriptor const &desc)
{
	_descriptor = desc;
	if (!_descriptor._window || any(lessThanEqual(glm::uvec3(_descriptor._dimensions), glm::uvec3(0)))) {
		ASSERT(0);
		return false;
	}
	ASSERT(_descriptor._dimensions[3] == 0);

	return true;
}

}
