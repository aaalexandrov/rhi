#pragma once

namespace rhi {

using utl::TypeInfo;
using utl::Enum;

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

