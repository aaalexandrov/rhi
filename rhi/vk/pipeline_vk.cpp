#include "pipeline_vk.h"
#include "rhi_vk.h"

#include "utl/mathutl.h"

#include "shaderc/shaderc.hpp"
#include "spirv_cross/spirv_reflect.hpp"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("pipeline_vk", [] {
	TypeInfo::Register<ShaderVk>().Name("ShaderVk")
		.Base<Shader>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());

	TypeInfo::Register<PipelineVk>().Name("PipelineVk")
		.Base<Pipeline>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());
});

vk::DescriptorType GetDescriptorType(ShaderParam::Kind kind)
{
	switch (kind) {
		case ShaderParam::UniformBuffer:
			return vk::DescriptorType::eUniformBuffer;
		case ShaderParam::UAVBuffer:
			return vk::DescriptorType::eStorageBuffer;
		case ShaderParam::Texture:
			return vk::DescriptorType::eSampledImage;
		case ShaderParam::UAVTexture:
			return vk::DescriptorType::eStorageImage;
		case ShaderParam::Sampler:
			return vk::DescriptorType::eSampler;
	}
	ASSERT(0);
	return vk::DescriptorType();
}

void DescriptorSetAllocatorVk::Set::Delete()
{
	if (!_allocator)
		return;
	{
		std::lock_guard lock(_allocator->_mutex);
		_allocator->_rhi->_device.freeDescriptorSets(_pool, _set);
	}
	_allocator = nullptr;
	_set = nullptr;
	_pool = nullptr;
}

DescriptorSetAllocatorVk::~DescriptorSetAllocatorVk()
{
	for (auto pool : _pools) {
		_rhi->_device.destroyDescriptorPool(pool, _rhi->AllocCallbacks());
	}
}

bool DescriptorSetAllocatorVk::Init(RhiVk *rhi, uint32_t baseDescriptorCount)
{
	ASSERT(!_rhi);
	ASSERT(_poolSizes.empty());
	_rhi = rhi;
	_maxSets = baseDescriptorCount;

	for (uint32_t i = 0; i < (uint32_t)ShaderParam::Kind::Count; ++i) {
		vk::DescriptorPoolSize size{
			GetDescriptorType((ShaderParam::Kind)i),
			baseDescriptorCount,
		};
		_poolSizes.push_back(size);
	}

	if (!AllocPool())
		return false;

	return true;
}

bool DescriptorSetAllocatorVk::Init(RhiVk *rhi, uint32_t maxSets, std::span<vk::DescriptorSetLayoutBinding> bindings)
{
	ASSERT(!_rhi);
	ASSERT(_poolSizes.empty());
	_rhi = rhi;
	_maxSets = maxSets;

	for (auto &bind : bindings) {
		auto it = std::find_if(_poolSizes.begin(), _poolSizes.end(), [&](auto &sz) {
			return sz.type == bind.descriptorType;
		});
		vk::DescriptorPoolSize &poolSize = it != _poolSizes.end() ? *it : _poolSizes.emplace_back();
		poolSize.type = bind.descriptorType;
		poolSize.descriptorCount += bind.descriptorCount * _maxSets;
	}

	if (!AllocPool())
		return false;

	return true;
}

auto DescriptorSetAllocatorVk::Allocate(vk::DescriptorSetLayout layout) -> Set
{
	Set set;
	vk::DescriptorSetAllocateInfo setInfo{
		nullptr,
		1,
		&layout
	};

	std::lock_guard lock(_mutex);

	ASSERT(_lastUsedPool < _pools.size());
	uint32_t startPool = _lastUsedPool;
	bool poolCreated = false;
	while (true) {
		setInfo.descriptorPool = _pools[_lastUsedPool];
		vk::Result res = _rhi->_device.allocateDescriptorSets(&setInfo, &set._set);
		if (res == vk::Result::eSuccess) {
			set._allocator = this;
			set._pool = _pools[_lastUsedPool];
			return set;
		} 
		if (poolCreated) {
			LOG("Failed to allocate descriptor set!");
			return set;
		}
		_lastUsedPool = (_lastUsedPool + 1) % _pools.size();
		if (_lastUsedPool == startPool) {
			AllocPool();
			poolCreated = true;
		}
	}
}

bool DescriptorSetAllocatorVk::AllocPool()
{
	vk::DescriptorPoolCreateInfo poolInfo{
		vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
		_maxSets,
		_poolSizes,
	};
	auto poolRes = _rhi->_device.createDescriptorPool(poolInfo, _rhi->AllocCallbacks());
	if (poolRes.result != vk::Result::eSuccess)
		return false;
	
	_lastUsedPool = (uint32_t)_pools.size();
	_pools.push_back(poolRes.value);

	return true;
}


static std::unordered_map<ShaderKind, vk::ShaderStageFlags> s_shaderKind2Vk{
	{ ShaderKind::Vertex, vk::ShaderStageFlagBits::eVertex },
	{ ShaderKind::Fragment, vk::ShaderStageFlagBits::eFragment },
	{ ShaderKind::Compute, vk::ShaderStageFlagBits::eCompute },
};


ShaderVk::~ShaderVk()
{
	auto rhi = static_cast<RhiVk *>(_rhi);
	rhi->_device.destroyShaderModule(_shaderModule, rhi->AllocCallbacks());
}

ShaderParam GetShaderParam(spirv_cross::Compiler const &refl, spirv_cross::Resource const &res, ShaderParam::Kind kind)
{
	auto getName = [&](spirv_cross::ID id) {
		std::string name = refl.get_name(id);
		if (name.empty())
			name = refl.get_fallback_name(id);
		return name;
	};

	auto getDescSet = [&](spirv_cross::ID id) {
		return kind == ShaderParam::VertexLayout 
			? 0
			: refl.get_decoration(res.id, spv::Decoration::DecorationDescriptorSet);
	};

	auto getBinding = [&](spirv_cross::ID id) {
		return refl.get_decoration(
			res.id, 
			kind == ShaderParam::VertexLayout 
				? spv::Decoration::DecorationLocation 
				: spv::Decoration::DecorationBinding);
	};

	ShaderParam param{
		._name = getName(res.id),
		._kind = kind,
		._set = getDescSet(res.id),
		._binding = getBinding(res.id),
	};

	std::function<TypeInfo const *(spirv_cross::SPIRType const &)> getType;
	getType = [&](spirv_cross::SPIRType const &type) {
		TypeInfo const *typeInfo = nullptr;
		switch (type.basetype) {
			case spirv_cross::SPIRType::Image:
				typeInfo = TypeInfo::Get<Texture>();
				// more data about the image can be found inside res.image
				break;
			case spirv_cross::SPIRType::Sampler:
				typeInfo = TypeInfo::Get<Sampler>();
				break;
			case spirv_cross::SPIRType::Struct: {
				param._ownTypes.emplace_back(std::make_unique<TypeInfo>());
				TypeInfo *structInfo = param._ownTypes.back().get();
				structInfo->_name = getName(type.self);
				structInfo->_size = refl.get_declared_struct_size(type);
				for (uint32_t i = 0; i < type.member_types.size(); ++i) {
					auto &memberType = refl.get_type(type.member_types[i]);
					auto &memberName = refl.get_member_name(type.self, i);
					size_t memberSize = refl.get_declared_struct_member_size(type, i);
					size_t memberOffset = refl.type_struct_member_offset(type, i);
					TypeInfo const *memberTypeInfo = getType(memberType);

					structInfo->_members.push_back({ ._name = memberName, ._var = {._type = memberTypeInfo, ._offset = memberOffset } });
				}
				if (structInfo->_members.size()) {
					ASSERT(structInfo->_members[0]._var._offset == 0);
					structInfo->_align = structInfo->_members[0]._var._type->_align;
				}
				typeInfo = structInfo;
				break;
			}
			case spirv_cross::SPIRType::SByte: {
				static std::unordered_map<std::pair<uint32_t, uint32_t>, TypeInfo const *> types{
					{ {1, 1}, TypeInfo::Get<int8_t>() },
					{ {1, 2}, TypeInfo::Get<glm::i8vec2>() },
					{ {1, 3}, TypeInfo::Get<glm::i8vec3>() },
					{ {1, 4}, TypeInfo::Get<glm::i8vec4>() },
				};
				typeInfo = types[{type.columns, type.vecsize}];
				break;
			}
			case spirv_cross::SPIRType::UByte: {
				static std::unordered_map<std::pair<uint32_t, uint32_t>, TypeInfo const *> types{
					{ {1, 1}, TypeInfo::Get<uint8_t>() },
					{ {1, 2}, TypeInfo::Get<glm::u8vec2>() },
					{ {1, 3}, TypeInfo::Get<glm::u8vec3>() },
					{ {1, 4}, TypeInfo::Get<glm::u8vec4>() },
				};
				typeInfo = types[{type.columns, type.vecsize}];
				break;
			}
			case spirv_cross::SPIRType::Int: {
				static std::unordered_map<std::pair<uint32_t, uint32_t>, TypeInfo const *> types{
					{ {1, 1}, TypeInfo::Get<int32_t>() },
					{ {1, 2}, TypeInfo::Get<glm::ivec2>() },
					{ {1, 3}, TypeInfo::Get<glm::ivec3>() },
					{ {1, 4}, TypeInfo::Get<glm::ivec4>() },
				};
				typeInfo = types[{type.columns, type.vecsize}];
				break;
			}
			case spirv_cross::SPIRType::UInt: {
				static std::unordered_map<std::pair<uint32_t, uint32_t>, TypeInfo const *> types{
					{ {1, 1}, TypeInfo::Get<uint32_t>() },
					{ {1, 2}, TypeInfo::Get<glm::uvec2>() },
					{ {1, 3}, TypeInfo::Get<glm::uvec3>() },
					{ {1, 4}, TypeInfo::Get<glm::uvec4>() },
				};
				typeInfo = types[{type.columns, type.vecsize}];
				break;
			}
			case spirv_cross::SPIRType::Float: {
				static std::unordered_map<std::pair<uint32_t, uint32_t>, TypeInfo const *> types{
					{ {1, 1}, TypeInfo::Get<float>() },
					{ {1, 2}, TypeInfo::Get<glm::vec2>() },
					{ {1, 3}, TypeInfo::Get<glm::vec3>() },
					{ {1, 4}, TypeInfo::Get<glm::vec4>() },

					{ {2, 2}, TypeInfo::Get<glm::mat2>() },
					{ {3, 3}, TypeInfo::Get<glm::mat3>() },
					{ {4, 4}, TypeInfo::Get<glm::mat4>() },
				};
				typeInfo = types[{type.columns, type.vecsize}];
				break;
			}
		}
		ASSERT(typeInfo);

		for (size_t i = 0; i < type.array.size(); ++i) {
			ASSERT(type.array_size_literal[i] && "Arrays with specialization constants not supported");

			param._ownTypes.emplace_back(std::make_unique<TypeInfo>());
			TypeInfo *arrayType = param._ownTypes.back().get();

			arrayType->_bases.push_back({ ._type = typeInfo, ._offset = 0 });
			arrayType->_isArray = true;
			arrayType->_arraySize = type.array[i];
			arrayType->_align = typeInfo->_align;
			arrayType->_size = typeInfo->_size * arrayType->_arraySize;

			typeInfo = arrayType;
		}

		return typeInfo;
	};

	spirv_cross::SPIRType const &resType = refl.get_type(res.type_id);
	param._type = getType(resType);

	return param;
}

bool ShaderVk::Load(std::string name, ShaderKind kind, std::vector<uint8_t> const &content)
{
	if (!Shader::Load(name, kind, content))
		return false;

	std::span<const uint32_t> spirv;
	shaderc::SpvCompilationResult shadercResult;

	static constexpr uint32_t s_SPIRVMagic = 0x07230203;
	bool isSpirv = content.size() >= sizeof(uint32_t) && content.size() % sizeof(uint32_t) == 0 && *(uint32_t *)content.data() == s_SPIRVMagic;
	if (!isSpirv) {
		static std::unordered_map<ShaderKind, shaderc_shader_kind> s_shaderKind2Shaderc = {
			{ ShaderKind::Vertex, shaderc_shader_kind::shaderc_vertex_shader },
			{ ShaderKind::Fragment, shaderc_shader_kind::shaderc_fragment_shader },
			{ ShaderKind::Compute, shaderc_shader_kind::shaderc_compute_shader },
		};
		ASSERT(s_shaderKind2Shaderc.size() == (size_t)ShaderKind::Count);

		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		shaderc_shader_kind shadercKind = s_shaderKind2Shaderc[_kind];
		shadercResult = compiler.CompileGlslToSpv((char const *)content.data(), content.size(), shadercKind, _name.c_str(), _entryPoint.c_str(), options);
		if (shadercResult.GetCompilationStatus() != shaderc_compilation_status_success) {
			LOG("Compilation of GLSL shader '{}' failed with error: {}", _name, shadercResult.GetErrorMessage());
			return false;
		}
		spirv = std::span(shadercResult.begin(), shadercResult.end());
	} else {
		spirv = std::span((uint32_t *)content.data(), content.size() / sizeof(uint32_t));
	}

	auto rhi = static_cast<RhiVk *>(_rhi);

	vk::ShaderModuleCreateInfo modInfo{
		vk::ShaderModuleCreateFlags(),
		spirv,
	};
	if (rhi->_device.createShaderModule(&modInfo, rhi->AllocCallbacks(), &_shaderModule) != vk::Result::eSuccess)
		return false;

	spirv_cross::Compiler refl(spirv.data(), spirv.size());
	auto shaderResources = refl.get_shader_resources(refl.get_active_interface_variables());
	auto addParams = [&](auto &resources, ShaderParam::Kind kind) {
		for (auto &res : resources) {
			_params.push_back(GetShaderParam(refl, res, kind));
		}
	};

	addParams(shaderResources.uniform_buffers, ShaderParam::UniformBuffer);
	addParams(shaderResources.storage_buffers, ShaderParam::UAVBuffer);
	addParams(shaderResources.separate_images, ShaderParam::Texture);
	addParams(shaderResources.storage_images, ShaderParam::UAVTexture);
	addParams(shaderResources.separate_samplers, ShaderParam::Sampler);

	if (_kind == ShaderKind::Vertex) {
		// Aggregate all vertex stage inputs into a common typeinfo
		ShaderParam attribs{
			._name = "#VertexLayout",
			._kind = ShaderParam::VertexLayout,
		};
		attribs._ownTypes.push_back(std::make_unique<TypeInfo>());
		TypeInfo *attribsType = attribs._ownTypes.back().get();
		attribs._type = attribsType;

		size_t offs = 0;
		for (auto &res : shaderResources.stage_inputs) {
			ShaderParam attr = GetShaderParam(refl, res, ShaderParam::VertexLayout);
			offs = utl::RoundUp(offs, attr._type->_align);
			attribsType->_members.push_back({ ._name = attr._name, ._var = { ._type = attr._type, ._offset = offs } });
			attribsType->_members.back()._metadata.push_back(utl::AnyValue::New(attr._binding));
			attribs._ownTypes.insert(attribs._ownTypes.end(), std::make_move_iterator(attr._ownTypes.begin()), std::make_move_iterator(attr._ownTypes.end()));
			offs += attr._type->_size;
		}

		attribsType->_size = offs;
		if (attribsType->_members.size())
			attribsType->_align = attribsType->_members[0]._var._type->_align;

		_params.push_back(std::move(attribs));
	}

	return true;
}

PipelineVk::~PipelineVk()
{
	auto rhi = static_cast<RhiVk *>(_rhi);
	rhi->_device.destroyPipeline(_pipeline, rhi->AllocCallbacks());
	rhi->_device.destroyPipelineLayout(_layout, rhi->AllocCallbacks());
	for (auto &setData : _descriptorSetData) {
		rhi->_device.destroyDescriptorSetLayout(setData._layout, rhi->AllocCallbacks());
	}
}

bool PipelineVk::Init(std::span<std::shared_ptr<Shader>> shaders)
{
	if (!Pipeline::Init(shaders))
		return false;

	if (!InitLayout())
		return false;

	ASSERT(_layout);

	auto rhi = static_cast<RhiVk *>(_rhi);

	if (_shaders.size() == 1 && _shaders[0]->_kind == ShaderKind::Compute) {
		auto shaderVk = static_cast<ShaderVk *>(_shaders[0].get());
		vk::ComputePipelineCreateInfo pipeInfo{
			vk::PipelineCreateFlags(),
			vk::PipelineShaderStageCreateInfo{
				vk::PipelineShaderStageCreateFlags(),
				vk::ShaderStageFlagBits::eCompute,
				shaderVk->_shaderModule,
				shaderVk->_entryPoint.c_str(),
			},
			_layout,
		};
		if (rhi->_device.createComputePipelines(rhi->_pipelineCache, 1, &pipeInfo, rhi->AllocCallbacks(), &_pipeline) != vk::Result::eSuccess)
			return false;

		return true;
	} 

	ASSERT(0);
	return false;
}

bool PipelineVk::InitLayout()
{
	ASSERT(s_shaderKind2Vk.size() == (size_t)ShaderKind::Count);
	auto rhi = static_cast<RhiVk *>(_rhi);

	_descriptorSetData.resize(_resourceSetDescriptions.size());
	std::vector<vk::DescriptorSetLayout> setLayouts(_resourceSetDescriptions.size());
	for (uint32_t setIndex = 0; setIndex < _resourceSetDescriptions.size(); ++setIndex) {
		auto &setDesc = _resourceSetDescriptions[setIndex];

		std::vector<vk::DescriptorSetLayoutBinding> bindings;
		for (uint32_t resIndex = 0; resIndex < setDesc._resources.size(); ++resIndex) {
			auto &resource = setDesc._resources[resIndex];
			vk::DescriptorSetLayoutBinding bind;
			bind.binding = resIndex;
			bind.descriptorType = GetDescriptorType(resource._kind);
			bind.descriptorCount = resource._numEntries;
			for (uint32_t i = 0; i < (uint32_t)ShaderKind::Count; ++i) {
				if (resource._shaderKindsMask & (1 << i))
					bind.stageFlags |= s_shaderKind2Vk[(ShaderKind)i];
			}
			bindings.push_back(bind);
		}

		vk::DescriptorSetLayoutCreateInfo setInfo{
			vk::DescriptorSetLayoutCreateFlags(),
			bindings,
		};
		auto setResult = rhi->_device.createDescriptorSetLayout(setInfo, rhi->AllocCallbacks());
		if (setResult.result != vk::Result::eSuccess)
			return false;

		setLayouts[setIndex] = _descriptorSetData[setIndex]._layout = setResult.value;
		_descriptorSetData[setIndex]._allocator = std::make_unique<DescriptorSetAllocatorVk>();
		if (!_descriptorSetData[setIndex]._allocator->Init(rhi, 1024, bindings))
			return false;
	}

	std::array<vk::PushConstantRange, 0> noPushConsts;
	vk::PipelineLayoutCreateInfo layoutInfo{
		vk::PipelineLayoutCreateFlags(),
		setLayouts,
		noPushConsts,
	};
	if (rhi->_device.createPipelineLayout(&layoutInfo, rhi->AllocCallbacks(), &_layout) != vk::Result::eSuccess)
		return false;

	return true;
}


}