#pragma once

#include "base.h"

namespace rhi {

struct Resource;
struct GraphicsPass;

struct ShaderParam {
	enum Kind: int8_t {
		Invalid = -1,
		UniformBuffer,
		UAVBuffer,
		Texture,
		UAVTexture,
		Sampler,
		VertexLayout,
		Count
	};

	uint32_t GetNumEntries() const {
		return std::max((uint32_t)_type->_arraySize, 1u);
	}

	std::string _name;
	TypeInfo const *_type = nullptr;
	Kind _kind = Invalid;
	uint32_t _set = ~0u, _binding = ~0u;
	std::vector<std::shared_ptr<TypeInfo>> _ownTypes;
};

struct Shader : public RhiOwned {
	virtual bool Load(ShaderData const &shaderData, std::vector<uint8_t> const &content);

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Shader>(); }

	uint32_t GetNumParams(ShaderParam::Kind kind) const;
	ShaderParam const *GetParam(ShaderParam::Kind kind, uint32_t index = 0) const;

	ShaderData GetShaderData() const;

	ShaderKind _kind = ShaderKind::Invalid;
	std::vector<ShaderParam> _params;
	glm::ivec3 _groupSize{ 0 };
};

struct ResourceSetDescription {
	struct Param {
		std::string _name;
		ShaderParam::Kind _kind = ShaderParam::Invalid;
		uint32_t _numEntries = 0;
		uint32_t _shaderKindsMask = 0;

		bool IsImage() const;
		bool IsBuffer() const;
		bool IsSampler() const;

		ResourceUsage GetUsage() const;
	};

	uint32_t GetNumEntries() const;

	std::vector<Param> _params;
};

struct Pipeline;
struct ResourceSet : public utl::Any {
	virtual ~ResourceSet() {}

	virtual bool Init(Pipeline *pipeline, uint32_t setIndex);

	virtual bool Update();

	bool Update(std::initializer_list<ResourceRef> resRefs) { return Update<std::initializer_list>(resRefs); }
	bool Update(std::span<ResourceRef> resRefs) { return Update<std::span>(resRefs); }

	template <template<typename> typename Container>
	bool Update(Container<ResourceRef> resRefs);

	ResourceSetDescription const *GetSetDescription() const;

	void EnumResources(ResourceEnum enumFn);

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<ResourceSet>(); }

	Pipeline *_pipeline = nullptr;
	uint32_t _setIndex = ~0u;
	std::vector<ResourceRef> _resourceRefs;
};

struct Pipeline : public RhiOwned {
	virtual bool Init(PipelineData const &pipelineData, GraphicsPass *renderPass = nullptr);

	virtual std::shared_ptr<ResourceSet> AllocResourceSet(uint32_t setIndex) = 0;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Pipeline>(); }

	PipelineData _pipelineData;
	std::vector<ResourceSetDescription> _resourceSetDescriptions;
};

inline ResourceSetDescription const *ResourceSet::GetSetDescription() const {
	return &_pipeline->_resourceSetDescriptions[_setIndex];
}

template<template<typename> typename Container>
inline bool ResourceSet::Update(Container<ResourceRef> resRefs)
{
	if (resRefs.size() != _resourceRefs.size()) {
		ASSERT(0);
		return false;
	}

	std::copy(resRefs.begin(), resRefs.end(), _resourceRefs.begin());

	return Update();
}

}
