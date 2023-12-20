#pragma once

#include "base.h"
#include "utl/enumutl.h"

namespace rhi {

struct SamplerDescriptor {
	Filter _minFilter = Filter::Linear, _magFilter = Filter::Linear;
	MipMapMode _mipMapMode = MipMapMode::Linear;
	std::array<AddressMode, 3> _addressModes = { AddressMode::Repeat, AddressMode::Repeat, AddressMode::Repeat };
	float _mipLodBias = 0;
	float _maxAnisotropy = 0;
	CompareOp _compareOp = CompareOp::Always;
	float _minLod = 0, _maxLod = 1000;
};

struct SwapchainDescriptor {
	ResourceUsage _usage{.rt = 1, .present = 1};
	Format _format = Format::Invalid;
	glm::ivec4 _dimensions{ 0 };
	PresentMode _presentMode = PresentMode::Fifo;
	std::shared_ptr<WindowData> _window;
};

struct Resource : public RhiOwned {
	ResourceDescriptor _descriptor;
	ResourceUsage _state{};

	virtual bool Init(ResourceDescriptor const &desc);

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Resource>(); }
};

struct Buffer : public Resource {

	size_t GetSize() const { return _descriptor._dimensions[0]; }

	virtual std::span<uint8_t> Map() = 0;
	virtual bool Unmap() = 0;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Buffer>(); }
};

struct Texture : public Resource {

	bool Init(ResourceDescriptor const &desc) override;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Texture>(); }

	std::weak_ptr<RhiOwned> _owner;
};

struct Sampler : public RhiOwned {
	SamplerDescriptor _descriptor;

	virtual bool Init(SamplerDescriptor const &desc);

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Sampler>(); }
};

struct Swapchain : public RhiOwned {
	SwapchainDescriptor _descriptor;

	virtual bool Init(SwapchainDescriptor const &desc);

	virtual std::vector<Format> GetSupportedSurfaceFormats() const = 0;
	virtual uint32_t GetSupportedPresentModeMask() const = 0;

	virtual bool Update(PresentMode presentMode = PresentMode::Invalid, Format surfaceFormat = Format::Invalid) = 0;

	virtual std::shared_ptr<Texture> AcquireNextImage() = 0;
	int32_t GetTextureIndex(Texture *tex) const;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Swapchain>(); }

	std::vector<std::shared_ptr<Texture>> _images;
};

}