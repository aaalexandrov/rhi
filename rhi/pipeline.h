#pragma once

#include "base.h"

namespace rhi {

struct Shader : public RhiOwned {


	virtual bool Load(std::string name, ShaderKind kind, std::vector<uint8_t> const &content) = 0;
	virtual bool Load(std::string name, ShaderKind kind, std::string path);

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Shader>(); }
};

struct Pipeline : public RhiOwned {

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Pipeline>(); }
};


}