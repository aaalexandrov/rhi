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

struct Pipeline : public RhiOwned {

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Pipeline>(); }
};


}