#include "base.h"
#include "rhi.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("type_info", [] {
	TypeInfo::Register<WindowData>().Name("WindowData")
		.Base<utl::Any>();

	TypeInfo::Register<RhiOwned>().Name("RhiOwned")
		.Base<utl::Any>();
});


RhiOwned::RhiOwned() 
{
}

RhiOwned::~RhiOwned() 
{
}

void RhiOwned::InitRhi(Rhi *rhi, std::string name)
{
	ASSERT(_rhi.expired());
	ASSERT(_name.empty());
	_rhi = rhi->weak_from_this();
	_name = name;
}

}
