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
	~PipelineVk() override;

	bool Init(std::span<std::shared_ptr<Shader>> shaders) override;

	bool InitLayout();

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<PipelineVk>(); }

	vk::Pipeline _pipeline;
	std::vector<vk::DescriptorSetLayout> _descriptorSetLayouts;
	vk::PipelineLayout _layout;
};

}