#include "pipeline_vk.h"
#include "rhi_vk.h"
#include "buffer_vk.h"
#include "texture_vk.h"
#include "sampler_vk.h"
#include "graphics_pass_vk.h"

#include "utl/mathutl.h"

#include "shaderc/shaderc.hpp"
#include "spirv_cross/spirv_reflect.hpp"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("pipeline_vk", [] {
	TypeInfo::Register<ShaderVk>().Name("ShaderVk")
		.Base<Shader>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());

	TypeInfo::Register<ResourceSetVk>().Name("ResourceSetVk")
		.Base<ResourceSet>();

	TypeInfo::Register<PipelineVk>().Name("PipelineVk")
		.Base<Pipeline>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());
});

utl::ValueRemapper<FrontFaceMode, vk::FrontFace> s_frontFace2Vk{ {
		{FrontFaceMode::CCW, vk::FrontFace::eCounterClockwise},
		{FrontFaceMode::CW, vk::FrontFace::eClockwise},
	} };

utl::ValueRemapper<CullMask, vk::CullModeFlagBits> s_cullMask2Vk{ {
		{CullMask::None, vk::CullModeFlagBits::eNone},
		{CullMask::Front, vk::CullModeFlagBits::eFront},
		{CullMask::Back, vk::CullModeFlagBits::eBack},
		{CullMask::FrontAndBack, vk::CullModeFlagBits::eFrontAndBack},
	} };

utl::ValueRemapper<CompareOp, vk::CompareOp> s_compareFunc2Vk{ {
		{CompareOp::Never, vk::CompareOp::eNever},
		{CompareOp::Less, vk::CompareOp::eLess},
		{CompareOp::Equal, vk::CompareOp::eEqual},
		{CompareOp::LessOrEqual, vk::CompareOp::eLessOrEqual},
		{CompareOp::Greater, vk::CompareOp::eGreater},
		{CompareOp::NotEqual, vk::CompareOp::eNotEqual},
		{CompareOp::GreaterOrEqual, vk::CompareOp::eGreaterOrEqual},
		{CompareOp::Always, vk::CompareOp::eAlways},
	} };

utl::ValueRemapper<StencilOp, vk::StencilOp> s_stencilFunc2Vk{ {
		{StencilOp::Keep, vk::StencilOp::eKeep},
		{StencilOp::Zero, vk::StencilOp::eZero},
		{StencilOp::Replace, vk::StencilOp::eReplace},
		{StencilOp::IncrementAndClamp, vk::StencilOp::eIncrementAndClamp},
		{StencilOp::DecrementAndClamp, vk::StencilOp::eDecrementAndClamp},
		{StencilOp::Invert, vk::StencilOp::eInvert},
		{StencilOp::IncrementAndWrap, vk::StencilOp::eIncrementAndWrap},
		{StencilOp::DecrementAndWrap, vk::StencilOp::eDecrementAndWrap},
	} };

utl::ValueRemapper<BlendOp, vk::BlendOp> s_blendFunc2Vk{ {
		{BlendOp::Add,             vk::BlendOp::eAdd,             },
		{BlendOp::Subtract,        vk::BlendOp::eSubtract,        },
		{BlendOp::ReverseSubtract, vk::BlendOp::eReverseSubtract, },
		{BlendOp::Min,             vk::BlendOp::eMin,             },
		{BlendOp::Max,             vk::BlendOp::eMax,             },
	} };

utl::ValueRemapper<BlendFactor, vk::BlendFactor> s_blendFactor2Vk{ {
		{BlendFactor::Zero,                     vk::BlendFactor::eZero,                  },
		{BlendFactor::One,                      vk::BlendFactor::eOne,                   },
		{BlendFactor::SrcColor,                 vk::BlendFactor::eSrcColor,              },
		{BlendFactor::OneMinusSrcColor,         vk::BlendFactor::eOneMinusSrcColor,      },
		{BlendFactor::DstColor,                 vk::BlendFactor::eDstColor,              },
		{BlendFactor::OneMinusDstColor,         vk::BlendFactor::eOneMinusDstColor,      },
		{BlendFactor::SrcAlpha,                 vk::BlendFactor::eSrcAlpha,              },
		{BlendFactor::OneMinusSrcAlpha,         vk::BlendFactor::eOneMinusSrcAlpha,      },
		{BlendFactor::DstAlpha,                 vk::BlendFactor::eDstAlpha,              },
		{BlendFactor::OneMinusDstAlpha,         vk::BlendFactor::eOneMinusDstAlpha,      },
		{BlendFactor::ConstantColor,            vk::BlendFactor::eConstantColor,         },
		{BlendFactor::OneMinusConstantColor,    vk::BlendFactor::eOneMinusConstantColor, },
		{BlendFactor::ConstantAlpha,            vk::BlendFactor::eConstantAlpha,         },
		{BlendFactor::OneMinusConstantAlpha,    vk::BlendFactor::eOneMinusConstantAlpha, },
		{BlendFactor::SrcAlphaSaturate,         vk::BlendFactor::eSrcAlphaSaturate,      },
		{BlendFactor::Src1Color,                vk::BlendFactor::eSrc1Color,             },
		{BlendFactor::OneMinusSrc1Color,        vk::BlendFactor::eOneMinusSrc1Color,     },
		{BlendFactor::Src1Alpha,                vk::BlendFactor::eSrc1Alpha,             },
		{BlendFactor::OneMinusSrc1Alpha,        vk::BlendFactor::eOneMinusSrc1Alpha,     },
	} };

utl::ValueRemapper<ColorComponentMask, vk::ColorComponentFlagBits, true> s_colorComponentMask2Vk{ {
		{ ColorComponentMask::R, vk::ColorComponentFlagBits::eR	},
		{ ColorComponentMask::G, vk::ColorComponentFlagBits::eG	},
		{ ColorComponentMask::B, vk::ColorComponentFlagBits::eB	},
		{ ColorComponentMask::A, vk::ColorComponentFlagBits::eA	},
	} };

utl::ValueRemapper<PrimitiveKind, vk::PrimitiveTopology> s_primitiveKind2VkTopology = { {
		{ PrimitiveKind::TriangleList, vk::PrimitiveTopology::eTriangleList },
		{ PrimitiveKind::TriangleStrip, vk::PrimitiveTopology::eTriangleStrip },
	} };


vk::Viewport GetVkViewport(Viewport const &viewport)
{
	vk::Viewport vkViewport;
	vkViewport
		.setX(viewport._rect._min.x)
		.setY(viewport._rect._min.y)
		.setWidth(viewport._rect.GetSize().x)
		.setHeight(viewport._rect.GetSize().y)
		.setMinDepth(viewport._minDepth)
		.setMaxDepth(viewport._maxDepth);
	return vk::Viewport();
}

void FillViewports(RenderState const &data, std::vector<vk::Viewport> &viewports)
{
	viewports.emplace_back(GetVkViewport(data._viewport));
}

void FillStencilOpState(StencilFuncState const &src, vk::StencilOpState &dst)
{
	dst
		.setFailOp(s_stencilFunc2Vk.ToDst(src._failFunc))
		.setPassOp(s_stencilFunc2Vk.ToDst(src._passFunc))
		.setDepthFailOp(s_stencilFunc2Vk.ToDst(src._depthFailFunc))
		.setCompareOp(s_compareFunc2Vk.ToDst(src._compareFunc))
		.setWriteMask(src._writeMask)
		.setReference(src._reference)
		.setCompareMask(src._compareMask);
}

void FillViewportState(RenderState const &data, vk::PipelineViewportStateCreateInfo &viewportState, std::vector<vk::Viewport> &viewports, std::vector<vk::Rect2D> &scissors)
{
	FillViewports(data, viewports);

	scissors.emplace_back();
	scissors.back()
		.setOffset(vk::Offset2D(data._scissor._min.x, data._scissor._min.y))
		.setExtent(vk::Extent2D(data._scissor.GetSize().x, data._scissor.GetSize().y));

	viewportState
		.setViewportCount(1)
		.setPViewports(&viewports.back())
		.setScissorCount(1)
		.setPScissors(&scissors.back());
}

void FillRasterizationState(RenderState const &data, vk::PipelineRasterizationStateCreateInfo &rasterState)
{
	rasterState
		.setCullMode(s_cullMask2Vk.ToDst(data._cullState._cullMask))
		.setFrontFace(s_frontFace2Vk.ToDst(data._cullState._front))
		.setDepthBiasEnable(data._depthBias._enable)
		.setDepthBiasConstantFactor(data._depthBias._constantFactor)
		.setDepthBiasClamp(data._depthBias._clamp)
		.setDepthBiasSlopeFactor(data._depthBias._slopeFactor)
		.setLineWidth(1.0f);
}

void FillMultisampleState(RenderState const &data, vk::PipelineMultisampleStateCreateInfo &multisampleState, std::vector<uint32_t> &sampleMask)
{
	multisampleState
		.setMinSampleShading(1.0f);
}

void FillDepthStencilState(RenderState const &data, vk::PipelineDepthStencilStateCreateInfo &depthState)
{
	depthState
		.setDepthTestEnable(data._depthState._depthTestEnable)
		.setDepthWriteEnable(data._depthState._depthWriteEnable)
		.setDepthCompareOp(s_compareFunc2Vk.ToDst(data._depthState._depthCompareFunc))
		.setDepthBoundsTestEnable(data._depthState._depthBoundsTestEnable)
		.setMinDepthBounds(data._depthState._minDepthBounds)
		.setMaxDepthBounds(data._depthState._maxDepthBounds)
		.setStencilTestEnable(data._stencilEnable);
	FillStencilOpState(data._stencilState[0], depthState.front);
	FillStencilOpState(data._stencilState[1], depthState.back);
}

void FillBlendState(RenderState const &data, vk::PipelineColorBlendStateCreateInfo &blendState, std::vector<vk::PipelineColorBlendAttachmentState> &attachmentBlends)
{
	for (auto &blend : data._blendStates) {
		attachmentBlends.resize(attachmentBlends.size() + 1);
		attachmentBlends.back()
			.setBlendEnable(blend._blendEnable)
			.setSrcColorBlendFactor(s_blendFactor2Vk.ToDst(blend._srcColorBlendFactor))
			.setDstColorBlendFactor(s_blendFactor2Vk.ToDst(blend._dstColorBlendFactor))
			.setColorBlendOp(s_blendFunc2Vk.ToDst(blend._colorBlendFunc))
			.setSrcAlphaBlendFactor(s_blendFactor2Vk.ToDst(blend._srcAlphaBlendFactor))
			.setDstAlphaBlendFactor(s_blendFactor2Vk.ToDst(blend._dstAlphaBlendFactor))
			.setAlphaBlendOp(s_blendFunc2Vk.ToDst(blend._alphaBlendFunc))
			.setColorWriteMask(s_colorComponentMask2Vk.ToDst(blend._colorWriteMask));
	}

	blendState
		.setAttachmentCount(static_cast<uint32_t>(attachmentBlends.size()))
		.setPAttachments(attachmentBlends.data())
		.setBlendConstants({ data._blendColor[0], data._blendColor[1], data._blendColor[2], data._blendColor[3] });
}

void FillDynamicState(RenderState const &data, vk::PipelineDynamicStateCreateInfo &dynamicState, std::vector<vk::DynamicState> &dynamicStates)
{
	dynamicStates.push_back(vk::DynamicState::eViewport);

	dynamicState
		.setDynamicStateCount(static_cast<uint32_t>(dynamicStates.size()))
		.setPDynamicStates(dynamicStates.data());
}


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


static std::unordered_map<ShaderKind, vk::ShaderStageFlagBits> s_shaderKind2Vk{
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
		std::string name = res.name;
		if (name.empty())
			name = refl.get_name(id);
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
				param._ownTypes.emplace_back(std::make_shared<TypeInfo>());
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

			param._ownTypes.emplace_back(std::make_shared<TypeInfo>());
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

bool ShaderVk::Load(ShaderData const &shaderData, std::vector<uint8_t> const &content)
{
	if (!Shader::Load(shaderData, content))
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
			LOG("Compilation of GLSL shader '%s' failed with error: %s", _name, shadercResult.GetErrorMessage());
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

	if (_kind == ShaderKind::Compute) {
		auto &entryPoint = refl.get_entry_point(_entryPoint, spv::ExecutionModelGLCompute);
		auto &groupSize = entryPoint.workgroup_size;
		_groupSize = glm::ivec3(groupSize.x, groupSize.y, groupSize.z);
		// group size might be dependent on specialization constants or be otherwise dynamic, and then dimensions might be 0
		ASSERT(all(greaterThan(_groupSize, glm::ivec3(0))));
	}

	if (_kind == ShaderKind::Vertex) {
		// Aggregate all vertex stage inputs into a common typeinfo
		ShaderParam attribs{
			._name = "#VertexLayout",
			._kind = ShaderParam::VertexLayout,
		};
		attribs._ownTypes.push_back(std::make_shared<TypeInfo>());
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

ResourceSetVk::~ResourceSetVk()
{
	ClearCreatedViews();
}

bool ResourceSetVk::Init(Pipeline *pipeline, uint32_t setIndex)
{
	if (!ResourceSet::Init(pipeline, setIndex))
		return false;

	auto pipeVk = static_cast<PipelineVk *>(_pipeline);
	_descSet = pipeVk->_descriptorSetData[_setIndex].AllocateDescSet();

	if (!_descSet)
		return false;

	return true;

}

bool ResourceSetVk::Update()
{
	if (!ResourceSet::Update())
		return false;

	ClearCreatedViews();

	ASSERT(_descSet);

	auto pipeVk = static_cast<PipelineVk *>(_pipeline);
	ResourceSetDescription const *setDescription = GetSetDescription();
	ASSERT(_resourceRefs.size() == setDescription->GetNumEntries());

	auto *rhi = static_cast<RhiVk *>(pipeVk->_rhi);
	std::vector<vk::WriteDescriptorSet> writeRes;
	// we pre-allocate the following arrays with worst-case size because we'll be storing pointers to their elements and we don't want them to get reallocated
	std::vector<vk::DescriptorImageInfo> imgInfos(_resourceRefs.size());
	std::vector<vk::DescriptorBufferInfo> bufInfos(_resourceRefs.size());
	std::array<vk::BufferView, 0> noTexelInfos;
	uint32_t resRefIdx = 0;
	for (uint32_t i = 0; i < setDescription->_params.size(); ++i) {
		auto &res = setDescription->_params[i];
		for (uint32_t e = 0; e < res._numEntries; ++e) {
			auto &resRef = _resourceRefs[resRefIdx + e];
			if (BufferVk *bufVk = Cast<BufferVk>(resRef._bindable.get())) {
				auto &bufInfo = bufInfos[resRefIdx + e];
				bufInfo.buffer = bufVk->_buffer;
				bufInfo.offset = resRef._view._region._min[0];
				bufInfo.range = resRef._view._region.GetSize()[0];
				if (bufInfo.offset + bufInfo.range > bufVk->GetSize()) {
					ASSERT(0);
					return false;
				}
			} else if (TextureVk *texVk = Cast<TextureVk>(resRef._bindable.get())) {
				auto &imgInfo = imgInfos[resRefIdx + e];
				// TO DO: handle resource ref views
				ResourceView defaultView = ResourceView::FromDescriptor(texVk->_descriptor, 0);
				if (resRef._view == defaultView) {
					imgInfo.imageView = texVk->_view;
				} else {
					imgInfo.imageView = texVk->CreateView(resRef._view);
					_createdViews.push_back(imgInfo.imageView);
				}
				ASSERT(imgInfo.imageView);
				ASSERT(res._kind == ShaderParam::UAVTexture || res._kind == ShaderParam::Texture);
				imgInfo.imageLayout = res._kind == ShaderParam::UAVTexture ? vk::ImageLayout::eGeneral : vk::ImageLayout::eShaderReadOnlyOptimal;
			} else if (SamplerVk *sampler = Cast<SamplerVk>(resRef._bindable.get())) {
				ASSERT(res._kind == ShaderParam::Sampler);
				auto &imgInfo = imgInfos[resRefIdx + e];
				imgInfo.sampler = sampler->_sampler;
			} else {
				ASSERT(0);
				return false;
			}
		}
		vk::WriteDescriptorSet write{
			_descSet._set,
			i, 
			0,
			res._numEntries,
			GetDescriptorType(res._kind),
			res.IsImage() || res.IsSampler() ? &imgInfos[resRefIdx] : nullptr,
			res.IsBuffer() ? &bufInfos[resRefIdx] : nullptr,
			nullptr,
		};
		writeRes.push_back(write);

		resRefIdx += res._numEntries;
	}

	std::array<vk::CopyDescriptorSet, 0> noCopies;
	rhi->_device.updateDescriptorSets(writeRes, noCopies);

	return true;
}

void ResourceSetVk::ClearCreatedViews()
{
	if (!_descSet)
		return;
	auto *rhi = _descSet._allocator->_rhi;
	for (auto view : _createdViews) {
		rhi->_device.destroyImageView(view, rhi->AllocCallbacks());
	}
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

vk::PipelineShaderStageCreateInfo GetShaderStageInfo(Shader *shader)
{
	auto shaderVk = static_cast<ShaderVk *>(shader);
	return vk::PipelineShaderStageCreateInfo{
			vk::PipelineShaderStageCreateFlags(),
			s_shaderKind2Vk[shaderVk->_kind],
			shaderVk->_shaderModule,
			shaderVk->_entryPoint.c_str(),
	};
}

vk::Format GetVertexAttribFormat(TypeInfo::Member const &attribMember)
{
	static std::unordered_map<TypeInfo const *, vk::Format> s_type2VkFormat{
		{ TypeInfo::Get<float>()      , vk::Format::eR32Sfloat          },
		{ TypeInfo::Get<glm::vec2>()  , vk::Format::eR32G32Sfloat       },
		{ TypeInfo::Get<glm::vec3>()  , vk::Format::eR32G32B32Sfloat    },
		{ TypeInfo::Get<glm::vec4>()  , vk::Format::eR32G32B32A32Sfloat },

		{ TypeInfo::Get<uint8_t>()    , vk::Format::eR8Unorm            },
		{ TypeInfo::Get<glm::u8vec2>(), vk::Format::eR8G8Unorm          },
		{ TypeInfo::Get<glm::u8vec3>(), vk::Format::eR8G8B8Unorm        },
		{ TypeInfo::Get<glm::u8vec4>(), vk::Format::eR8G8B8A8Unorm      },
	};

	auto it = s_type2VkFormat.find(attribMember._var._type);
	if (it != s_type2VkFormat.end())
		return it->second;
	ASSERT(0);
	return vk::Format::eUndefined;
}

bool PipelineVk::Init(PipelineData const &pipelineData, GraphicsPass *renderPass)
{
	if (!Pipeline::Init(pipelineData, renderPass))
		return false;

	if (!InitLayout())
		return false;

	ASSERT(_layout);

	auto rhi = static_cast<RhiVk *>(_rhi);

	if (pipelineData.IsCompute()) {
		vk::ComputePipelineCreateInfo pipeInfo{
			vk::PipelineCreateFlags(),
			GetShaderStageInfo(_pipelineData._shaders[0].get()),
			_layout,
		};
		if (rhi->_device.createComputePipelines(rhi->_pipelineCache, 1, &pipeInfo, rhi->AllocCallbacks(), &_pipeline) != vk::Result::eSuccess)
			return false;

	} else {
		Shader *vertexShader = _pipelineData.GetShader(ShaderKind::Vertex);
		if (!vertexShader)
			return false;
		ASSERT(vertexShader->GetNumParams(ShaderParam::Kind::VertexLayout) <= 1);
		ShaderParam const *vertShaderLayout = vertexShader->GetParam(ShaderParam::Kind::VertexLayout, 0);

		std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
		for (auto &shader : _pipelineData._shaders)
			shaderStages.push_back(GetShaderStageInfo(shader.get()));

		std::vector<vk::VertexInputBindingDescription> vertInputBinds;
		std::vector<vk::VertexInputAttributeDescription> vertInputAttrs;
		if (vertShaderLayout) {
			for (uint32_t binding = 0; binding < _pipelineData._vertexInputs.size(); ++binding) {
				auto &vertInput = _pipelineData._vertexInputs[binding];
				bool addedBind = false;
				for (auto &inputAttrib : vertInput._layout->_members) {
					uint32_t const *attrLocation = vertShaderLayout->_type->GetMemberMetadata<uint32_t>(inputAttrib._name);
					if (!attrLocation)
						continue;
					if (!addedBind) {
						vertInputBinds.push_back(vk::VertexInputBindingDescription{
							binding,
							(uint32_t)vertInput._layout->_size,
							vertInput._perInstance ? vk::VertexInputRate::eInstance : vk::VertexInputRate::eVertex
							});
						addedBind = true;
					}
					vertInputAttrs.push_back(vk::VertexInputAttributeDescription{
						*attrLocation,
						binding,
						GetVertexAttribFormat(inputAttrib),
						(uint32_t)inputAttrib._var._offset
						});
				}
			}
		}

		vk::PipelineVertexInputStateCreateInfo vertInputInfo{
			vk::PipelineVertexInputStateCreateFlags(),
			vertInputBinds,
			vertInputAttrs,
		};

		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo{
			vk::PipelineInputAssemblyStateCreateFlags(),
			s_primitiveKind2VkTopology.ToDst(_pipelineData._primitiveKind),
			false,
		};

		std::vector<vk::Viewport> viewports;
		std::vector<vk::Rect2D> scissors;
		vk::PipelineViewportStateCreateInfo viewportState;
		FillViewportState(_pipelineData._renderState, viewportState, viewports, scissors);

		vk::PipelineRasterizationStateCreateInfo rasterizationState;
		FillRasterizationState(_pipelineData._renderState, rasterizationState);

		std::vector<uint32_t> sampleMask;
		vk::PipelineMultisampleStateCreateInfo multisampleState;
		FillMultisampleState(_pipelineData._renderState, multisampleState, sampleMask);

		vk::PipelineDepthStencilStateCreateInfo depthStencilState;
		FillDepthStencilState(_pipelineData._renderState, depthStencilState);

		std::vector<vk::PipelineColorBlendAttachmentState> attachmentBlends;
		vk::PipelineColorBlendStateCreateInfo blendState;
		FillBlendState(_pipelineData._renderState, blendState, attachmentBlends);

		std::vector<vk::DynamicState> dynamicStates;
		vk::PipelineDynamicStateCreateInfo dynamicState;
		FillDynamicState(_pipelineData._renderState, dynamicState, dynamicStates);

		auto *graphicsPassVk = static_cast<GraphicsPassVk *>(renderPass);
		vk::GraphicsPipelineCreateInfo pipeInfo{
			vk::PipelineCreateFlags(),
			shaderStages,
			&vertInputInfo,
			&inputAssemblyInfo,
			nullptr,
			&viewportState,
			&rasterizationState,
			&multisampleState,
			&depthStencilState,
			&blendState,
			&dynamicState,
			_layout,
			graphicsPassVk->_renderPass,
			0,
			nullptr,
			0,
		};
		if (rhi->_device.createGraphicsPipelines(rhi->_pipelineCache, 1, &pipeInfo, rhi->AllocCallbacks(), &_pipeline) != vk::Result::eSuccess)
			return false;
	}

	rhi->SetDebugName(vk::ObjectType::ePipeline, (uint64_t)(VkPipeline)_pipeline, _name.c_str());

	return true;
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
		for (uint32_t resIndex = 0; resIndex < setDesc._params.size(); ++resIndex) {
			auto &resource = setDesc._params[resIndex];
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

std::shared_ptr<ResourceSet> PipelineVk::AllocResourceSet(uint32_t setIndex)
{
	auto resSet = std::make_shared<ResourceSetVk>();
	if (!resSet->Init(this, setIndex))
		return std::shared_ptr<ResourceSet>();
	return resSet;
}


}