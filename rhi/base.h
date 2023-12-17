#pragma once

namespace rhi {

using utl::TypeInfo;
using utl::Enum;

union ResourceUsage {
	struct {
		uint32_t vb : 1;
		uint32_t ib : 1;
		uint32_t srv : 1;
		uint32_t uav : 1;
		uint32_t rt : 1;
		uint32_t ds : 1;
		uint32_t present : 1;
		uint32_t copySrc : 1;
		uint32_t copyDst : 1;
		uint32_t cpuAccess : 1;
		uint32_t read : 1;
		uint32_t write : 1;
		uint32_t cube : 1;
	};
	uint32_t _flags = 0;

	friend inline ResourceUsage operator&(ResourceUsage u0, ResourceUsage u1) { return ResourceUsage{ ._flags = u0._flags & u1._flags }; }
	friend inline ResourceUsage operator|(ResourceUsage u0, ResourceUsage u1) { return ResourceUsage{ ._flags = u0._flags | u1._flags }; }
	ResourceUsage operator~() { return ResourceUsage{ ._flags = ~_flags }; }
	ResourceUsage &operator|=(ResourceUsage u) { _flags |= u._flags; return *this; }
	ResourceUsage &operator&=(ResourceUsage u) { _flags &= u._flags; return *this; }
	bool operator==(ResourceUsage u) const { return _flags == u._flags; }
	bool operator!=(ResourceUsage u) const { return _flags != u._flags; }

	operator bool() { return _flags; }
	bool operator!() { return !_flags; }
};


enum class Format : int32_t {
	Invalid = -1,

	R8,

	R8G8B8A8,
	R8G8B8A8_srgb,
	B8G8R8A8,
	B8G8R8A8_srgb,

	D32,
	D32S8,
	D24S8,
	S8,

	Count,

	DepthFirst = D32,
	DepthLast = D24S8,
	StencilFirst = D32S8,
	StencilLast = S8,

	DepthStencilFirst = DepthFirst,
	DepthStencilLast = StencilLast,
};

inline bool IsDepth(Format fmt) {
	return Format::DepthFirst <= fmt && fmt <= Format::DepthLast;
}

inline bool IsStencil(Format fmt) {
	return Format::StencilFirst <= fmt && fmt <= Format::StencilLast;
}

inline bool IsDepthStencil(Format fmt) {
	return Format::DepthStencilFirst <= fmt && fmt <= Format::DepthStencilLast;
}

enum class PresentMode : int8_t {
	Invalid = -1,
	Immediate,
	Mailbox,
	Fifo,
	FifoRelaxed,
	Count,
};

enum class Filter : int8_t {
	Nearest,
	Linear,
	Cubic,
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

struct WindowData : public utl::Any {
	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<WindowData>(); }
};

struct Rhi;
struct RhiOwned : public std::enable_shared_from_this<RhiOwned>, public utl::Any {
	std::weak_ptr<Rhi> _rhi;
	std::string _name;

	RhiOwned();
	virtual ~RhiOwned();

	virtual bool InitRhi(Rhi *rhi, std::string name);

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<RhiOwned>(); }

	static inline std::string s_rhiTagType{ "rhi" };
};

} // rhi

