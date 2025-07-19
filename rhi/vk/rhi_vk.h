#pragma once

#include "base_vk.h"

#include "../rhi.h"

namespace rhi {

#if defined(_WIN32)
struct WindowDataWin32 : public WindowData {
	HINSTANCE _hinstance = nullptr;
	HWND _hwnd = nullptr;
	WindowDataWin32(HINSTANCE hinstance, HWND hwnd) : _hinstance(hinstance), _hwnd(hwnd) {}

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<WindowDataWin32>(); }
};
#elif defined(__linux__)
struct WindowDataXlib : public WindowData {
	Display *_display = nullptr;     /**< The X11 display */
	Window _window = 0;              /**< The X11 window */
	WindowDataXlib(Display *display, Window window) : _display(display), _window(window) {}
	VisualID GetVisualId() const {
		XWindowAttributes winAttr;
		XGetWindowAttributes(_display, _window, &winAttr);
		return XVisualIDFromVisual(winAttr.visual);
	}

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<WindowDataXlib>(); }
};

struct WindowDataWayland : public WindowData {
	struct wl_display *_display = nullptr;
	struct wl_surface *_surface = nullptr;
	WindowDataWayland(struct wl_display *display, struct wl_surface *surface) : _display(display), _surface(surface) {}

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<WindowDataWayland>(); }
};
#endif

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

struct RhiVk final : public Rhi {

	~RhiVk() override;

	std::vector<DeviceDescription> GetDevices(Settings const &settings) override;

	bool Init(Settings const &settings, int32_t deviceIndex = 0) override;

	bool WaitIdle() override;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<RhiVk>(); }

	vk::AllocationCallbacks *AllocCallbacks() { return _allocTracker ? &_allocTracker->_allocCallbacks : nullptr; }

	bool InitInstance();
	bool InitDevice(int32_t deviceIndex);
	bool InitVma();

	bool SetDebugName(vk::ObjectType objType, uint64_t handle, char const *name);

	std::span<uint32_t> GetQueueFamilyIndices(ResourceUsage usage);
	ResourceUsage GetFormatImageUsage(Format fmt, ResourceUsage usage);
	vk::FormatFeatureFlags GetFormatFeatures(Format fmt, ResourceUsage usage);

	VmaAllocationCreateInfo GetVmaAllocCreateInfo(Resource *resource);

	// The host allocation tracker's callbacks will be called during destruction of Vulkan objects
	// so the tracker has to appear before all those variables in the class, so it gets desroyed after them
	std::unique_ptr<HostAllocationTrackerVk> _allocTracker;
	vk::DebugUtilsMessengerEXT _debugUtilsMessenger;

	struct QueueData {
		vk::Queue _queue;
		uint32_t _family = ~0;
	};

	vk::Instance _instance;
	vk::detail::DispatchLoaderDynamic _dynamicDispatch;
	vk::PhysicalDevice _physDevice;
	vk::Device _device;
	QueueData _universalQueue;
	VmaAllocator _vma = {};
	TimelineSemaphoreVk _timelineSemaphore;
	vk::PipelineCache _pipelineCache;
};

}