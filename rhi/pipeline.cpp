#include "pipeline.h"
#include "pass.h"
#include "resource.h"

#include "utl/file.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("pipeline", [] {
	TypeInfo::Register<Shader>().Name("Shader")
		.Base<RhiOwned>();

	TypeInfo::Register<ResourceSet>().Name("ResourceSet")
		.Base<utl::Any>();

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

uint32_t Shader::GetNumParams(ShaderParam::Kind kind) const
{
	uint32_t num = 0;
	for (auto &param : _params) {
		if (kind == ShaderParam::Kind::Invalid || param._kind == kind)
			++num;
	}
	return num;
}

ShaderParam const *Shader::GetParam(ShaderParam::Kind kind, uint32_t index) const
{
	for (auto &param : _params) {
		if (kind != ShaderParam::Kind::Invalid && param._kind != kind)
			continue;
		if (!index)
			return &param;
		--index;
	}
	return nullptr;
}

uint32_t ResourceSetDescription::GetNumEntries() const
{
	uint32_t numEntries = 0;
	for (auto &res : _params) {
		ASSERT((res._numEntries > 0) == (res._kind != ShaderParam::Kind::VertexLayout));
		numEntries += res._numEntries;
	}
	return numEntries;
}

bool ResourceSetDescription::Param::IsImage() const
{
	return _kind == ShaderParam::Texture || _kind == ShaderParam::UAVTexture;
}

bool ResourceSetDescription::Param::IsBuffer() const
{
	return _kind == ShaderParam::UniformBuffer || _kind == ShaderParam::UAVBuffer;
}

bool ResourceSetDescription::Param::IsSampler() const 
{
	return _kind == ShaderParam::Sampler;
}

ResourceUsage ResourceSetDescription::Param::GetUsage() const
{
	switch (_kind) {
		case ShaderParam::UniformBuffer:
		case ShaderParam::Texture:
			return ResourceUsage{ .srv = 1, .read = 1 };
		case ShaderParam::UAVBuffer:
		case ShaderParam::UAVTexture:
			return ResourceUsage{ .uav = 1, .write = 1 };
			return ResourceUsage{ .srv = 1, .read = 1 };
	}
	return ResourceUsage();
}

bool ResourceSet::Init(Pipeline *pipeline, uint32_t setIndex)
{
	ASSERT(!_pipeline);
	_pipeline = pipeline;
	_setIndex = setIndex;
	_resourceRefs.resize(GetSetDescription()->GetNumEntries());

	return true;
}

bool ResourceSet::Update()
{
	for (uint32_t i = 0; i < _resourceRefs.size(); ++i) {
		if (!_resourceRefs[i].ValidateView()) {
			ASSERT(0);
			return false;
		}
	}
	return true;
}

bool ResourceSet::Update(std::initializer_list<ResourceRef> resRefs)
{
	if (resRefs.size() != _resourceRefs.size()) {
		ASSERT(0);
		return false;
	}

	std::copy(resRefs.begin(), resRefs.end(), _resourceRefs.begin());

	return Update();
}

void ResourceSet::EnumResources(ResourceEnum enumFn)
{
	ResourceSetDescription const *setDesc = GetSetDescription();
	uint32_t resIndex = 0;
	for (auto &param : setDesc->_params) {
		ResourceUsage paramUsage = param.GetUsage();
		if (paramUsage) {
			for (uint32_t e = 0; e < param._numEntries; ++e) {
				Resource *resource = Cast<Resource>(_resourceRefs[resIndex + e]._bindable.get());
				if (!resource)
					continue;
				enumFn(resource, paramUsage);
			}
		}
		resIndex += param._numEntries;
	}
	ASSERT(resIndex == _resourceRefs.size());
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
			ResourceSetDescription::Param &paramDesc = utl::GetFromVec(setDesc._params, param._binding);
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

bool Pipeline::Init(GraphicsPipelineData &pipelineData)
{
	if (!Pipeline::Init(pipelineData._shaders))
		return false;

	if (!pipelineData._renderPass)
		return false;

	_renderState = std::make_unique<RenderState>(pipelineData._renderState);
	_vertexInputs = pipelineData._vertexInputs;
	_primitiveKind = pipelineData._primitiveKind;

	ASSERT(_renderTargetFormats.empty());
	for (auto &rt : pipelineData._renderPass->_renderTargets) {
		_renderTargetFormats.push_back(rt._texture->_descriptor._format);
	}

	return true;
}

glm::ivec3 Pipeline::GetComputeGroupSize() const
{
	Shader *compute = GetShader(ShaderKind::Compute);
	return compute ? compute->_groupSize : glm::ivec3(0);
}

Shader *Pipeline::GetShader(ShaderKind kind) const
{
	auto it = std::find_if(_shaders.begin(), _shaders.end(), [=](auto &s) { return s->_kind == kind; });
	return it != _shaders.end() ? it->get() : nullptr;
}

}