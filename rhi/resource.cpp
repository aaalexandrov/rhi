#include "resource.h"
#include "rhi.h"


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
		.Base<Resource>();
	TypeInfo::Register<Swapchain>().Name("Swapchain")
		.Base<Resource>();
});


bool Buffer::Init(ResourceDescriptor const &desc)
{
	_descriptor = desc;
	return true;
}

bool Texture::Init(ResourceDescriptor const &desc)
{
	_descriptor = desc;
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
