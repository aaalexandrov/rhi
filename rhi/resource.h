#pragma once

#include "base.h"
#include "utl/enumutl.h"

namespace rhi {

union ResourceUsage {
	struct {
		uint32_t vb : 1;
		uint32_t ib : 1;
		uint32_t srv : 1;
		uint32_t uav : 1;
		uint32_t rt : 1;
		uint32_t present : 1;
		uint32_t copySrc : 1;
		uint32_t copyDst : 1;
		uint32_t cpuAccess : 1;
		uint32_t read : 1;
		uint32_t write : 1;
	};
	uint32_t _flags = 0;
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
	ResourceUsage _usage;
	Format _format = Format::Invalid;
	glm::uvec4 _dimensions{ 0 };
};

struct SwapchainDescriptor : public ResourceDescriptor {
	std::shared_ptr<WindowData> _window;
	PresentMode _presentMode = PresentMode::Fifo;
};

struct Resource : public RhiOwned {
	ResourceDescriptor _descriptor;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Resource>(); }
};

struct Buffer : public Resource {
	ResourceDescriptor _descriptor;

	virtual bool Init(ResourceDescriptor const &desc);

	size_t GetSize() const { return _descriptor._dimensions[0]; }

	virtual std::span<uint8_t> Map(size_t offset = 0, size_t size = ~0ull) = 0;
	virtual bool Unmap() = 0;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Buffer>(); }
};

struct Texture : public Resource {
	ResourceDescriptor _descriptor;

	virtual bool Init(ResourceDescriptor const &desc);

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Texture>(); }
};

struct Sampler : public Resource {
	SamplerDescriptor _descriptor;

	virtual bool Init(SamplerDescriptor const &desc);

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Sampler>(); }
};

struct Swapchain : public Resource {
	SwapchainDescriptor _descriptor;

	virtual bool Init(SwapchainDescriptor const &desc);

	virtual std::vector<Format> GetSupportedSurfaceFormats() const = 0;
	virtual uint32_t GetSupportedPresentModeMask() const = 0;

	virtual bool Update(glm::uvec2 size, PresentMode presentMode = PresentMode::Invalid, Format surfaceFormat = Format::Invalid) = 0;

	virtual std::shared_ptr<Texture> AcquireNextImage() = 0;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Swapchain>(); }
};

}