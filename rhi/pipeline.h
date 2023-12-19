#pragma once

#include "base.h"

namespace rhi {

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
};


struct ResourceSetDescription {
	struct Param {
		std::string _name;
		ShaderParam::Kind _kind = ShaderParam::Invalid;
		uint32_t _numEntries = 0;
		uint32_t _shaderKindsMask = 0;
	};
	std::vector<Param> _resources;
};

struct Pipeline : public RhiOwned {

	virtual bool Init(std::span<std::shared_ptr<Shader>> shaders);

	Shader *GetShader(ShaderKind kind);

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Pipeline>(); }

	std::vector<std::shared_ptr<Shader>> _shaders;
	std::vector<ResourceSetDescription> _resourceSetDescriptions;
};


}