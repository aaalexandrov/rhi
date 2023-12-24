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
			return *this;
		}

		operator bool() const { return _allocator != nullptr; }

		vk::DescriptorSet _set;
		vk::DescriptorPool _pool;
		DescriptorSetAllocatorVk *_allocator = nullptr;
	};

	DescriptorSetAllocatorVk() = default;
	~DescriptorSetAllocatorVk();

	bool Init(RhiVk *rhi, uint32_t baseDescriptorCount);
	bool Init(RhiVk *rhi, uint32_t maxSets, std::span<vk::DescriptorSetLayoutBinding> bindings);

	DescriptorSetAllocatorVk &operator=(DescriptorSetAllocatorVk const &) = delete;
	DescriptorSetAllocatorVk &operator=(DescriptorSetAllocatorVk &&) = delete;

	Set Allocate(vk::DescriptorSetLayout layout);

	bool AllocPool();

	RhiVk *_rhi = nullptr;
	std::mutex _mutex;
	std::vector<vk::DescriptorPoolSize> _poolSizes;
	uint32_t _maxSets = 0;
	std::vector<vk::DescriptorPool> _pools;
	uint32_t _lastUsedPool = 0;
};
using DescSetVk = DescriptorSetAllocatorVk::Set;

struct ShaderVk : public Shader {
	~ShaderVk() override;

	bool Load(std::string name, ShaderKind kind, std::vector<uint8_t> const &content) override;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<ShaderVk>(); }

	vk::ShaderModule _shaderModule;
	std::string _entryPoint = "main";
};

struct ResourceSetVk : public ResourceSet {
	~ResourceSetVk() override;

	bool Init(Pipeline *pipeline, uint32_t setIndex) override;

	bool Update() override;

	void ClearCreatedViews();

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<ResourceSetVk>(); }

	DescSetVk _descSet;
	std::vector<vk::ImageView> _createdViews;
};

struct PipelineVk : public Pipeline {
	~PipelineVk() override;

	bool Init(std::span<std::shared_ptr<Shader>> shaders) override;
	bool Init(GraphicsPipelineData &pipelineData) override;

	bool InitLayout();

	std::shared_ptr<ResourceSet> AllocResourceSet(uint32_t setIndex) override;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<PipelineVk>(); }

	struct DescriptorSetData {
		vk::DescriptorSetLayout _layout;
		std::unique_ptr<DescriptorSetAllocatorVk> _allocator;

		DescSetVk AllocateDescSet() {
			return _allocator->Allocate(_layout);
		}
	};

	vk::Pipeline _pipeline;
	std::vector<DescriptorSetData> _descriptorSetData;
	vk::PipelineLayout _layout;
};

}