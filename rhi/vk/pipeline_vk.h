#pragma once

#include "base_vk.h"
#include "../pipeline.h"

namespace rhi {

struct ShaderVk : public Shader {
	~ShaderVk() override;

	bool Load(std::string name, ShaderKind kind, std::vector<uint8_t> const &content) override;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<ShaderVk>(); }

	vk::ShaderModule _shaderModule;
};

struct PipelineVk : public Pipeline {



	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<PipelineVk>(); }

};

}