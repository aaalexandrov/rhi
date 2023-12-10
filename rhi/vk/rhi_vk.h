#pragma once

#include "base_vk.h"

#include "../rhi.h"

namespace rhi {

struct HostAllocationTrackerVk {
	vk::AllocationCallbacks _allocCallbacks{ this, Allocate, Reallocate, Free, InternalAllocationNotify, InternalFreeNotify };

	size_t _allocations = 0;
	size_t _deallocations = 0;
	size_t _reallocations = 0;
	size_t _internalAllocations = 0;
	size_t _internalDeallocations = 0;

	size_t _sizeAllocated = 0;
	size_t _sizeAllocatedInternal = 0;

	static VKAPI_ATTR void *VKAPI_CALL Allocate(void *pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope);
	static VKAPI_ATTR void *VKAPI_CALL Reallocate(void *pUserData, void *pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope);
	static VKAPI_ATTR void VKAPI_CALL Free(void *pUserData, void *pMemory);
	static VKAPI_ATTR void VKAPI_CALL InternalAllocationNotify(void *pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope);
	static VKAPI_ATTR void VKAPI_CALL InternalFreeNotify(void *pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope);
};


struct RhiVk : public Rhi {

	bool Init(Settings const &settings) override;
	void Done() override;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<RhiVk>(); }

	vk::AllocationCallbacks *AllocCallbacks() { return _allocTracker ? &_allocTracker->_allocCallbacks : nullptr; }

	bool InitInstance();
	bool InitPhysicalDevice();

	// The host allocation tracker's callbacks will be called during destruction of Vulkan objects
	// so the tracker has to appear before all those variables in the class, so it gets desroyed after them
	std::unique_ptr<HostAllocationTrackerVk> _allocTracker;
	vk::DebugReportCallbackEXT _debugReportCallback;

	vk::Instance _instance;
	vk::DispatchLoaderDynamic _dynamicDispatch;
};

}