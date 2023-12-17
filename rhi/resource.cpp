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
	_state.create = 1;
	return true;
}

bool Texture::Init(ResourceDescriptor const &desc)
{
	if (!Resource::Init(desc))
		return false;
	ASSERT(_descriptor._dimensions[0] > 0);
	if (_descriptor._mipLevels == 0)
		_descriptor.SetMaxMipLevels();

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
	_descriptor._dimensions[3] = std::max(_descriptor._dimensions[3], 1u);
	_descriptor._usage.present = 1;
	if (!_descriptor._window) {
		ASSERT(0);
		return false;
	}

	return true;
}

int32_t Swapchain::GetTextureIndex(Texture *tex) const
{
	for (int32_t i = 0; i < _images.size(); ++i) {
		if (_images[i].get() == tex)
			return i;
	}
	return -1;
}

}
