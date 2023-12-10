#include "rhi_vk.h"
#include "utl/mathutl.h"
#include "utl/mem.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("rhi_vk", [] {
    TypeInfo::Register<RhiVk>().Name("RhiVk")
        .Base<Rhi>();
});


static bool IsAligned(void *mem, size_t alignment)
{
    ASSERT(mem != nullptr);
//    ASSERT(utl::IsPowerOf2(alignment));
    return !(reinterpret_cast<size_t>(mem) & (alignment - 1));
}

VKAPI_ATTR void *VKAPI_CALL HostAllocationTrackerVk::Allocate(void *pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
    HostAllocationTrackerVk *tracker = static_cast<HostAllocationTrackerVk *>(pUserData);

    void *mem = utl::AlignedAlloc(alignment, size);

    ASSERT(IsAligned(mem, alignment));

    ++tracker->_allocations;
    tracker->_sizeAllocated += utl::AlignedMemSize(mem);

    return mem;
}

VKAPI_ATTR void *VKAPI_CALL HostAllocationTrackerVk::Reallocate(void *pUserData, void *pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
    HostAllocationTrackerVk *tracker = static_cast<HostAllocationTrackerVk *>(pUserData);

    size_t prevSize = utl::AlignedMemSize(pOriginal);

    void *mem = utl::AlignedRealloc(pOriginal, alignment, size);
    ASSERT(IsAligned(mem, alignment));

    ++tracker->_reallocations;
    tracker->_sizeAllocated += utl::AlignedMemSize(mem) - prevSize;

    return mem;
}

VKAPI_ATTR void VKAPI_CALL HostAllocationTrackerVk::Free(void *pUserData, void *pMemory)
{
    HostAllocationTrackerVk *tracker = static_cast<HostAllocationTrackerVk *>(pUserData);

    ++tracker->_deallocations;
    tracker->_sizeAllocated -= utl::AlignedMemSize(pMemory);

    utl::AlignedFree(pMemory);
}

VKAPI_ATTR void VKAPI_CALL HostAllocationTrackerVk::InternalAllocationNotify(void *pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope)
{
    HostAllocationTrackerVk *tracker = static_cast<HostAllocationTrackerVk *>(pUserData);

    ++tracker->_internalAllocations;
    tracker->_sizeAllocatedInternal += size;
}

VKAPI_ATTR void VKAPI_CALL HostAllocationTrackerVk::InternalFreeNotify(void *pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope)
{
    HostAllocationTrackerVk *tracker = static_cast<HostAllocationTrackerVk *>(pUserData);

    ++tracker->_internalDeallocations;
    tracker->_sizeAllocatedInternal -= size;
}

VKAPI_ATTR VkBool32 VKAPI_CALL DbgReportFunc(
    VkFlags msgFlags,
    VkDebugReportObjectTypeEXT objType,
    uint64_t srcObject,
    size_t location,
    int32_t msgCode,
    const char *pLayerPrefix,
    const char *pMsg,
    void *pUserData)
{
    std::string flag = (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT) ? "error" : "warning";

    LOG("Vulkan debug ", flag, ", code: ", msgCode, ", layer: ", pLayerPrefix, ", msg: ", pMsg);

    return false;
}



bool RhiVk::Init(Settings const &settings)
{
    if (!Rhi::Init(settings))
        return false;

    if (!InitInstance())
        return false;

    return true;
}

bool RhiVk::InitInstance()
{
    if (_settings._enableValidation) {
        _allocTracker = std::make_unique<HostAllocationTrackerVk>();
    }

    vk::ApplicationInfo appInfo{
        _settings._appName,
        VK_MAKE_API_VERSION(0, _settings._appVersion[0], _settings._appVersion[1], _settings._appVersion[2]),
        "RhiVk",
        VK_MAKE_API_VERSION(0, 0, 1, 0),
        VK_API_VERSION_1_2
    };
    std::vector<const char *> instLayerNames = {};
    std::vector<const char *> instExtNames = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(_WIN32)
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined(__linux__)
        VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#endif
    };

    vk::DebugReportCallbackCreateInfoEXT debugCBInfo{
        vk::DebugReportFlagBitsEXT::eWarning | vk::DebugReportFlagBitsEXT::eError,
        DbgReportFunc,
        this
    };

    if (_settings._enableValidation) {
        instLayerNames.push_back("VK_LAYER_KHRONOS_validation");
        instExtNames.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }
    vk::InstanceCreateInfo inst{
        vk::InstanceCreateFlags(),
        &appInfo,
        instLayerNames,
        instExtNames,
        _settings._enableValidation ? &debugCBInfo : nullptr,
    };
    if (vk::createInstance(&inst, AllocCallbacks(), &_instance) != vk::Result::eSuccess)
        return false;

    _dynamicDispatch.init(_instance, vkGetInstanceProcAddr);

    if (_settings._enableValidation) {
        if (_instance.createDebugReportCallbackEXT(&debugCBInfo, AllocCallbacks(), &_debugReportCallback, _dynamicDispatch) != vk::Result::eSuccess)
            return false;
    }

    return true;
}

void RhiVk::Done()
{
    if (_debugReportCallback)
        _instance.destroyDebugReportCallbackEXT(_debugReportCallback, AllocCallbacks(), _dynamicDispatch);
    _instance.destroy(AllocCallbacks());

    Rhi::Done();
}


}