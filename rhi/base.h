#pragma once

namespace rhi {

using utl::TypeInfo;
using utl::Enum;

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

	DepthStencilFirst = D32,
	DepthStencilLast = S8,
};

enum class PresentMode : int8_t {
	Invalid = -1,
	Immediate,
	Mailbox,
	Fifo,
	FifoRelaxed,
	Count,
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

	void InitRhi(Rhi *rhi, std::string name);

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<RhiOwned>(); }

	static inline std::string s_rhiTagType{ "rhi" };
};

} // rhi

