#pragma once

#include "base_vk.h"
#include "../resource.h"

namespace rhi {

struct SwapchainVk : public Swapchain {


	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<SwapchainVk>(); }

};

}