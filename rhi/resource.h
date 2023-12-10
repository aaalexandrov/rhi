#pragma once

#include "base.h"

namespace rhi {

enum class Format : int32_t {
	Invalid = -1,

	R8G8B8A8,
	B8G8R8A8,

	D32,
	S8,
	D24S8,

	Count
};

enum class Filter : int8_t {
	Nearest,
	Linear,
};

enum class MipMapMode : int8_t {
	Nearest,
	Linear,
};

enum class AddressMode : int8_t {
	Repeat,
	MirroredRepeat,
	ClampToEdge,
};

enum class CompareOp : int8_t {
	Never,
	Less,
	Equal,
	LessOrEqual,
	Greater,
	NotEqual,
	GreaterOrEqual,
	Always,
};

struct SamplerDescriptor {
	Filter _minFilter = Filter::Linear, _magFilter = Filter::Linear;
	MipMapMode _mipMapMode = MipMapMode::Linear;
	std::array<AddressMode, 3> _addressModes = { AddressMode::Repeat, AddressMode::Repeat, AddressMode::Repeat };
	float _mipLodBias = 0;
	float _maxAnisotropy = 0;
	CompareOp _compareOp = CompareOp::Always;
	float _minLod = 0, _maxLod = 1000;
};

struct ResourceDescriptor {
	Format _format = Format::Invalid;
	glm::uvec4 _dimensions{ 0 };
};

struct Resource : public RhiOwned {
	ResourceDescriptor _descriptor;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Resource>(); }
};

struct Buffer : public Resource {

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Buffer>(); }
};

struct Texture : public Resource {

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Texture>(); }
};

struct Sampler : public Resource {
	
	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Sampler>(); }
};

struct Swapchain : public Resource {

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Swapchain>(); }
};

struct Pipeline : public Resource {

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Pipeline>(); }
};


}