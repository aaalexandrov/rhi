#pragma once

#include "base_vk.h"
#include "../pipeline.h"

namespace rhi {

struct DescriptorSetAllocatorVk {
	struct Set {
		Set() {}
		Set(Set &&other) { Swap(other); }
		~Set() { Delete(); }

		void Delete();

		void Swap(Set &other) {
			std::swap(_set, other._set);
			std::swap(_pool, other._pool);
			std::swap(_allocator, other._allocator);
		}

		Set &operator=(Set &&other) {
			if (this != &other) {
				Delete();
				Swap(other);
			}
		}

		operator bool() const { return _allocator != nullptr; }

		vk::DescriptorSet _set;
		vk::DescriptorPool _pool;
		DescriptorSetAllocatorVk *_allocator = nullptr;
	};

	bool Init(RhiVk *rhi, uint32_t baseDescriptorCount);

	Set Allocate(vk::DescriptorSetLayout layout);

	bool AllocPool();

	RhiVk *_rhi = nullptr;
	std::mutex _mutex;
	std::vector<vk::DescriptorPoolSize> _poolSizes;
	uint32_t _baseDescriptorCount = 0;
	std::vector<vk::DescriptorPool> _pools;
	uint32_t _lastUsedPool = 0;
};

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