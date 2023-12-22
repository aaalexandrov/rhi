#pragma once

#include "base.h"

namespace rhi {

struct Resource;

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
	std::vector<std::unique_ptr<TypeInfo>> _ownTypes;
};

struct Shader : public RhiOwned {
	virtual bool Load(std::string name, ShaderKind kind, std::vector<uint8_t> const &content);
	virtual bool Load(std::string path, ShaderKind kind);

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Shader>(); }

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

	bool Update(std::initializer_list<ResourceRef> resRefs);

	ResourceSetDescription const *GetSetDescription() const;

	void EnumResources(ResourceEnum enumFn);

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<ResourceSet>(); }

	Pipeline *_pipeline = nullptr;
	uint32_t _setIndex = ~0u;
	std::vector<ResourceRef> _resourceRefs;
};

struct Pipeline : public RhiOwned {

	virtual bool Init(std::span<std::shared_ptr<Shader>> shaders);

	virtual std::shared_ptr<ResourceSet> AllocResourceSet(uint32_t setIndex) = 0;

	Shader *GetShader(ShaderKind kind) const;

	glm::ivec3 GetComputeGroupSize() const;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Pipeline>(); }

	std::vector<std::shared_ptr<Shader>> _shaders;
	std::vector<ResourceSetDescription> _resourceSetDescriptions;
	std::unique_ptr<RenderState> _renderState;
};

inline ResourceSetDescription const *ResourceSet::GetSetDescription() const {
	return &_pipeline->_resourceSetDescriptions[_setIndex];
}

}