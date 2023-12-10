#include "swapchain_vk.h"
#include "rhi_vk.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("swapchain_vk", [] {
	TypeInfo::Register<SwapchainVk>().Name("SwapchainVk")
		.Base<Swapchain>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());
});


}