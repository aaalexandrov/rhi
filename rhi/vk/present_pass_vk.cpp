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

	ExecuteDataVk exec;
	exec._fnExecute = [this](RhiVk::QueueData &queue) {
		SwapchainVk *swapchain = static_cast<SwapchainVk *>(_swapchain.get());
		uint32_t imgIndex = swapchain->GetTextureIndex(_swapchainTexture.get());
		ASSERT(imgIndex < swapchain->_images.size());

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
		switch (ret) {
			case vk::Result::eSuboptimalKHR:
			case vk::Result::eErrorOutOfDateKHR:
				swapchain->_needsUpdate = true;
			case vk::Result::eSuccess:
				break;
			default:
				return false;
		}

		return true;
	};

	if (!subVk->Execute(std::move(exec)))
		return false;

	return true;
}

}