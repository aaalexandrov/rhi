#pragma once

#include "base.h"

namespace rhi {

struct Pass : public RhiOwned {
	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Pass>(); }
};

struct GraphicsPass : public Pass {
	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<GraphicsPass>(); }
};

struct ComputePass : public Pass {
	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<ComputePass>(); }
};

struct CopyPass : public Pass {
	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<CopyPass>(); }
};

struct PresentPass : public Pass {
	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<PresentPass>(); }
};

}