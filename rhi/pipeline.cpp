#include "pipeline.h"

#include "utl/file.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("pipeline", [] {
	TypeInfo::Register<Shader>().Name("Shader")
		.Base<RhiOwned>();

	TypeInfo::Register<Pipeline>().Name("Pipeline")
		.Base<RhiOwned>();
});

bool Shader::Load(std::string name, ShaderKind kind, std::string path)
{
	return Load(name, kind, utl::ReadFile(path));
}

}