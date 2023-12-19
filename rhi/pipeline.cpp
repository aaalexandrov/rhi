#include "pipeline.h"

#include "utl/file.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("pipeline", [] {
	TypeInfo::Register<Shader>().Name("Shader")
		.Base<RhiOwned>();

	TypeInfo::Register<Pipeline>().Name("Pipeline")
		.Base<RhiOwned>();
});

bool Shader::Load(std::string name, ShaderKind kind, std::vector<uint8_t> const &content)
{
	ASSERT(_name.empty());
	ASSERT(_kind == ShaderKind::Invalid);
	_name = name;
	_kind = kind;
	return true;
}

bool Shader::Load(std::string path, ShaderKind kind)
{
	return Load(utl::GetPathFilenameExt(path), kind, utl::ReadFile(path));
}

bool Pipeline::Init(std::span<std::shared_ptr<Shader>> shaders)
{
	ASSERT(_shaders.empty());
	for (auto &shader : shaders) {
		ASSERT(GetShader(shader->_kind) == nullptr);
		_shaders.push_back(shader);
	}

	return true;
}

Shader *Pipeline::GetShader(ShaderKind kind)
{
	auto it = std::find_if(_shaders.begin(), _shaders.end(), [=](auto &s) { return s->_kind == kind; });
	return it != _shaders.end() ? it->get() : nullptr;
}

}