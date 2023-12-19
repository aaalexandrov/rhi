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

		for (auto &param : shader->_params) {
			if (param._kind == ShaderParam::VertexLayout)
				continue;

			ResourceSetDescription &setDesc = utl::GetFromVec(_resourceSetDescriptions, param._set);
			ResourceSetDescription::Param paramDesc = utl::GetFromVec(setDesc._resources, param._binding);
			uint32_t numEntries = param.GetNumEntries();
			if (paramDesc._numEntries) {
				if (paramDesc._name != param._name || paramDesc._kind != param._kind || paramDesc._numEntries != numEntries) {
					LOG("Pipeline '%s' contains shader '%s' with parameter '%s' (%d entries) that doesn't match previous definitions, can't build resource set for the pipeline", _name, shader->_name, param._name, numEntries);
					return false;
				}
			} else {
				paramDesc._name = param._name;
				paramDesc._kind = param._kind;
				paramDesc._numEntries = numEntries;
			}
			paramDesc._shaderKindsMask |= 1u << (int8_t)shader->_kind;
		}
	}

	return true;
}

Shader *Pipeline::GetShader(ShaderKind kind)
{
	auto it = std::find_if(_shaders.begin(), _shaders.end(), [=](auto &s) { return s->_kind == kind; });
	return it != _shaders.end() ? it->get() : nullptr;
}

}