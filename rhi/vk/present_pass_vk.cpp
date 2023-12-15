#include "present_pass_vk.h"
#include "rhi_vk.h"
#include "submit_vk.h"
#include "swapchain_vk.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("present_pass_vk", [] {
	TypeInfo::Register<PresentPassVk>().Name("PresentPassVk")
		.Base<PresentPass>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());
});


bool PresentPassVk::Prepare(Submission *sub)
{
	return true;
}

bool PresentPassVk::Execute(Submission *sub)
{
	SubmissionVk *subVk = static_cast<SubmissionVk*>(sub);
	SwapchainVk *swapchain = static_cast<SwapchainVk*>(_swapchain.get());
	uint32_t imgIndex = swapchain->GetTextureIndex(_swapchainTexture.get());
	ASSERT(imgIndex < swapchain->_images.size());
	bool res = subVk->ExecuteDirect([&](RhiVk::QueueData &queue) {
		std::vector<vk::Semaphore> waitSemaphores;
		vk::PresentInfoKHR presentInfo{
			(uint32_t)waitSemaphores.size(),
			waitSemaphores.data(),
			1, 
			&swapchain->_swapchain,
			&imgIndex,
			&_presentResult,
		};
		vk::Result ret = queue._queue.presentKHR(presentInfo);
		if (ret != vk::Result::eSuccess && ret != vk::Result::eSuboptimalKHR)
			return false;

		return true;
	}, vk::PipelineStageFlagBits::eTransfer);
	if (!res)
		return false;

	return true;
}

}