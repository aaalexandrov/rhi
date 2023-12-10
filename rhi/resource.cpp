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
	TypeInfo::Register<Pipeline>().Name("Pipeline")
		.Base<Resource>();
});

}