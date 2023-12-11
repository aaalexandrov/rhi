#pragma once

#include "base.h"

namespace rhi {

struct Pipeline : public RhiOwned {

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Pipeline>(); }
};


}