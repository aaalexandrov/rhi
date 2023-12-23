#pragma once

#include "utl/enumutl.h"

namespace rhi {

using utl::TypeInfo;
using utl::Enum;

struct Resource;

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
		uint32_t create : 1;
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

	static inline constexpr ResourceUsage Operations() { return ResourceUsage{ .vb = 1, .ib = 1, .srv = 1, .uav = 1, .rt = 1, .ds = 1, .present = 1, .copySrc = 1, .copyDst = 1, .cpuAccess = 1 }; }
	static inline constexpr ResourceUsage Access() { return ResourceUsage{ .read = 1, .write = 1, .create = 1 }; }
};


enum class Format : int32_t {
	Invalid = -1,

	R8,

	R16_u,
	R32_u,

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

static inline std::unordered_map<Format, TypeInfo const *> s_format2TypeInfo{ {
		{ Format::R8, TypeInfo::Get<uint8_t>() },
		{ Format::R16_u, TypeInfo::Get<uint16_t>() },
		{ Format::R32_u, TypeInfo::Get<uint32_t>() },
		{ Format::R8G8B8A8, TypeInfo::Get<glm::uvec4>() },
		{ Format::R8G8B8A8_srgb, TypeInfo::Get<glm::uvec4>() },
		{ Format::B8G8R8A8, TypeInfo::Get<uint32_t>() },
		{ Format::B8G8R8A8_srgb, TypeInfo::Get<uint32_t>() },
		{ Format::D32, TypeInfo::Get<uint32_t>() },
		{ Format::S8, TypeInfo::Get<uint8_t>() },
	} };

uint32_t GetFormatSize(Format fmt);

uint8_t GetMaxMipLevels(glm::ivec3 dims);
glm::ivec3 GetMipLevelSize(glm::ivec3 dims, int32_t mipLevel);

struct ResourceDescriptor {
	ResourceUsage _usage;
	Format _format = Format::Invalid;
	glm::ivec4 _dimensions{ 0 };
	int8_t _mipLevels = 1;

	void SetMaxMipLevels() { _mipLevels = GetMaxMipLevels(_dimensions); }

	glm::ivec4 GetNaturalDims() const { return GetNaturalDims(_dimensions); }
	static glm::ivec4 GetNaturalDims(glm::ivec4 dims);

	bool IsCube() const { return IsCube(_dimensions); }
	static bool IsCube(glm::ivec4 dims);
};

struct ResourceView {
	Format _format = Format::Invalid;
	utl::Box4I _region = utl::Box4I::GetMaximum();
	utl::IntervalI8 _mipRange = utl::IntervalI8::GetMaximum();

	bool IsValidFor(ResourceDescriptor const &desc) const;
	ResourceView GetIntersection(ResourceView const &other) const;
	ResourceView GetIntersection(ResourceDescriptor &desc) const;
	static ResourceView FromDescriptor(ResourceDescriptor const &desc);
};

using ResourceEnum = std::function<void(Resource *, ResourceUsage)>;

struct ResourceRef {
	std::shared_ptr<Resource> _resource;
	ResourceView _view;

	bool ValidateView();
	bool IsViewValid();
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

enum class ShaderKind: int8_t {
	Invalid = -1,
	Vertex,
	Fragment,
	Compute,
	Count,
};

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

enum class StencilOp {
	Keep,
	Zero,
	Replace,
	IncrementAndClamp,
	DecrementAndClamp,
	Invert,
	IncrementAndWrap,
	DecrementAndWrap,
};

enum class BlendOp
{
	Add,
	Subtract,
	ReverseSubtract,
	Min,
	Max,
};

enum class BlendFactor
{
	Zero,
	One,
	SrcColor,
	OneMinusSrcColor,
	DstColor,
	OneMinusDstColor,
	SrcAlpha,
	OneMinusSrcAlpha,
	DstAlpha,
	OneMinusDstAlpha,
	ConstantColor,
	OneMinusConstantColor,
	ConstantAlpha,
	OneMinusConstantAlpha,
	SrcAlphaSaturate,
	Src1Color,
	OneMinusSrc1Color,
	Src1Alpha,
	OneMinusSrc1Alpha,
};

enum class ColorComponentMask
{
	None,
	R = 1,
	G = 2,
	B = 4,
	A = 8,
	RGBA = 15,
};

DEFINE_ENUM_BIT_OPERATORS(ColorComponentMask)


enum class FrontFaceMode {
	CCW,
	CW,
};

enum class CullMask {
	None,
	Front = 1,
	Back = 2,
	FrontAndBack = 3,
};


struct Viewport {
	utl::RectF _rect{ glm::vec2(0), glm::vec2(-1) };
	float _minDepth = 0, _maxDepth = 0;

	size_t GetHash() const;
	bool operator==(Viewport const &other) const;
};

struct CullState {
	FrontFaceMode _front = FrontFaceMode::CCW;
	CullMask _cullMask = CullMask::None;

	size_t GetHash() const;
	bool operator==(CullState const &other) const;
};

struct DepthBias {
	bool _enable = false;
	float _constantFactor = 0;
	float _clamp = 0;
	float _slopeFactor = 0;

	size_t GetHash() const;
	bool operator==(DepthBias const &other) const;
};

struct DepthState {
	bool _depthTestEnable = false;
	bool _depthWriteEnable = false;
	CompareOp _depthCompareFunc = CompareOp::Less;
	bool _depthBoundsTestEnable = false;
	float _minDepthBounds = 0.0f;
	float _maxDepthBounds = 1.0f;

	size_t GetHash() const;
	bool operator==(DepthState const &other) const;
};

struct StencilFuncState {
	StencilOp _failFunc = StencilOp::Keep;
	StencilOp _passFunc = StencilOp::Keep;
	StencilOp _depthFailFunc = StencilOp::Keep;
	CompareOp _compareFunc = CompareOp::Never;
	uint32_t _compareMask = 0;
	uint32_t _writeMask = 0;
	uint32_t _reference = 0;

	size_t GetHash() const;
	bool operator==(StencilFuncState const &other) const;
};

struct BlendFuncState {
	bool _blendEnable = false;
	BlendFactor _srcColorBlendFactor = BlendFactor::SrcAlpha;
	BlendFactor _dstColorBlendFactor = BlendFactor::OneMinusSrcAlpha;
	BlendOp _colorBlendFunc = BlendOp::Add;
	BlendFactor _srcAlphaBlendFactor = BlendFactor::SrcAlpha;
	BlendFactor _dstAlphaBlendFactor = BlendFactor::OneMinusSrcAlpha;
	BlendOp _alphaBlendFunc = BlendOp::Add;
	ColorComponentMask _colorWriteMask = ColorComponentMask::RGBA;

	size_t GetHash() const;
	bool operator==(BlendFuncState const &other) const;
};

struct RenderState {
	Viewport _viewport;
	utl::RectI _scissor{glm::ivec2(0), glm::ivec2(1024*1024)};
	CullState _cullState;
	DepthBias _depthBias;
	DepthState _depthState;
	bool _stencilEnable = false;
	std::array<StencilFuncState, 2> _stencilState;
	std::vector<BlendFuncState> _blendStates{ BlendFuncState() };
	glm::vec4 _blendColor{};

	size_t GetHash() const;
	bool operator==(RenderState const &other) const;
};

enum class PrimitiveKind {
	TriangleList,
	TriangleStrip,
};

struct WindowData : public utl::Any {
	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<WindowData>(); }
};

struct Rhi;
struct RhiOwned : public std::enable_shared_from_this<RhiOwned>, public utl::Any {
	Rhi *_rhi = nullptr;
	std::string _name;

	RhiOwned();
	virtual ~RhiOwned();

	virtual bool InitRhi(Rhi *rhi, std::string name);

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<RhiOwned>(); }

	static inline std::string s_rhiTagType{ "rhi" };
};

} // rhi

namespace std {

template<>
struct hash<rhi::ResourceUsage> {
	size_t operator()(rhi::ResourceUsage u) const {
		size_t h = hash<decltype(rhi::ResourceUsage::_flags)>()(u._flags);
		return h;
	}
};

template<>
struct hash<rhi::StencilFuncState> { size_t operator()(rhi::StencilFuncState const &s) { return s.GetHash(); } };

template<>
struct hash<rhi::BlendFuncState> { size_t operator()(rhi::BlendFuncState const &b) { return b.GetHash(); } };

template<>
struct hash<rhi::RenderState> { size_t operator()(rhi::RenderState const &r) { return r.GetHash(); } };

}