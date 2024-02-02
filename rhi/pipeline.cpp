#include "pipeline.h"
#include "pass.h"
#include "resource.h"
#include <bit>

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("pipeline", [] {
	TypeInfo::Register<Shader>().Name("Shader")
		.Base<RhiOwned>();

	TypeInfo::Register<ResourceSet>().Name("ResourceSet")
		.Base<utl::Any>();

	TypeInfo::Register<Pipeline>().Name("Pipeline")
		.Base<RhiOwned>();
});

bool Shader::Load(ShaderData const &shaderData, std::vector<uint8_t> const &content)
{
	ASSERT(_name.empty());
	ASSERT(_kind == ShaderKind::Invalid);
	_name = shaderData._name;
	_kind = shaderData._kind;
	return true;
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

ShaderParam const *Shader::GetParam(std::string name) const
{
	for (auto &param : _params) {
		if (param._name == name)
			return &param;
	}
	return nullptr;
}

ShaderData Shader::GetShaderData() const
{
	return ShaderData{ ._name = _name, ._kind = _kind };
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

int32_t ResourceSetDescription::GetParamIndex(std::string name, ShaderParam::Kind kind) const
{
	for (int32_t i = 0; i < _params.size(); ++i) {
		auto &param = _params[i];
		if (param._name == name && kind == ShaderParam::Invalid || param._kind == kind)
			return i;
	}
	return -1;
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

bool Pipeline::Init(PipelineData const &pipelineData, GraphicsPass *renderPass)
{
	ASSERT(_pipelineData.IsEmpty());
	_pipelineData = pipelineData;
	// FillRenderTargetFormats should already have been called
	ASSERT(_pipelineData._renderTargetFormats.size() == (renderPass ? renderPass->_renderTargets.size() : 0));
	for (auto &shader : _pipelineData._shaders) {
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

ShaderParam const *Pipeline::GetShaderParam(uint32_t setIndex, uint32_t bindingIndex)
{
	ASSERT(setIndex < _resourceSetDescriptions.size());
	ASSERT(bindingIndex < _resourceSetDescriptions[setIndex]._params.size());
	auto &setParam = _resourceSetDescriptions[setIndex]._params[bindingIndex];
	ShaderKind shaderKind = (ShaderKind)(std::bit_width(setParam._shaderKindsMask) - 1);
	ASSERT((1u << (int)shaderKind) & setParam._shaderKindsMask);
	Shader *shader = _pipelineData.GetShader(shaderKind);
	ASSERT(shader);
	ShaderParam const *param = shader->GetParam(setParam._name);
	ASSERT(param && param->_name == setParam._name && param->_set == setIndex && param->_binding == bindingIndex);

	return param;
}

ShaderParam const *Pipeline::GetShaderParam(uint32_t setIndex, std::string name, ShaderParam::Kind paramKind)
{
	ASSERT(setIndex < _resourceSetDescriptions.size());
	int32_t paramIndex = _resourceSetDescriptions[setIndex].GetParamIndex(name, paramKind);
	if (paramIndex < 0)
		return nullptr;
	return GetShaderParam(setIndex, paramIndex);
}

}