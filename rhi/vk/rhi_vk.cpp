#include "rhi_vk.h"
#include "utl/mathutl.h"
#include "utl/mem.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("rhi_vk", [] {
#if defined(_WIN32)
    TypeInfo::Register<WindowDataWin32>().Name("WindowDataWin32")
        .Base<WindowData>();
#elif defined(__linux__)
    TypeInfo::Register<WindowDataXlib>().Name("WindowDataXlib")
        .Base<WindowData>();
#endif

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

VKAPI_ATTR VkBool32 DebugUtilsMessengerFunc(
    VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData)
{
    char const *severity, *type;
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        severity = "error";
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        severity = "warning";
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        severity = "info";
    else {
        ASSERT(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT);
        severity = "verbose";
    }

    if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
        type = "general";
    else if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
        type = "validation";
    else if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
        type = "performance";
    else {
        ASSERT(messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT);
        type = "device address binding";
    }

    LOG("Vulkan %s %s, id: '%s', msg: '%s'", type, severity, pCallbackData->pMessageIdName, pCallbackData->pMessage);
    // TO DO: print queue / cmd messages & anything else the message can signify

    return false;
}

RhiVk::~RhiVk()
{
    ClearCachedData();

    _device.destroyPipelineCache(_pipelineCache, AllocCallbacks());
    vmaDestroyAllocator(_vma);
    _timelineSemaphore.Done();
    _device.destroy(AllocCallbacks());
    if (_debugUtilsMessenger)
        _instance.destroyDebugUtilsMessengerEXT(_debugUtilsMessenger, AllocCallbacks(), _dynamicDispatch);
    _instance.destroy(AllocCallbacks());
}

struct DeviceCreateData {
    vk::PhysicalDevice _physDevice;
    int32_t _universalQueueFamily = -1;
    std::vector<char const *> _layerNames, _extNames;
};

DeviceCreateData CheckPhysicalDeviceSuitability(vk::PhysicalDevice const &physDev, Rhi::Settings const &settings)
{
    auto queueFamilyCanPresent = [&](vk::PhysicalDevice const &physDev, int32_t queueFamily) {
#if defined(_WIN32)
        return physDev.getWin32PresentationSupportKHR(queueFamily);
#elif defined(__linux__)
        auto *winDataXlib = Cast<WindowDataXlib>(settings._window.get());
        ASSERT(winDataXlib);
        return winDataXlib && physDev.getXlibPresentationSupportKHR(queueFamily, winDataXlib->_display, winDataXlib->GetVisualId());
#endif
    };

    DeviceCreateData devCreateData;

    std::vector<char const *> extNames = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    auto devExts = physDev.enumerateDeviceExtensionProperties();
    if (devExts.result != vk::Result::eSuccess)
        return devCreateData;
    for (auto &name : extNames) {
        auto it = std::find_if(devExts.value.begin(), devExts.value.end(), [&](vk::ExtensionProperties const &ext) {
            return strcmp(ext.extensionName, name) == 0;
        });
        if (it == devExts.value.end())
            return devCreateData;
    }

    std::vector<vk::QueueFamilyProperties2> queueFamilies = physDev.getQueueFamilyProperties2();
    vk::QueueFlags universalFlags = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer;
    for (int32_t q = 0; q < queueFamilies.size(); ++q) {
        if ((queueFamilies[q].queueFamilyProperties.queueFlags & universalFlags) != universalFlags ||
            !queueFamilyCanPresent(physDev, q))
            continue;
        devCreateData._physDevice = physDev;
        devCreateData._universalQueueFamily = q;
        devCreateData._extNames = std::move(extNames);
        break;
    }
    return devCreateData;
}

auto RhiVk::GetDevices(Settings const &settings) -> std::vector<DeviceDescription>
{
    std::vector<DeviceDescription> devices;
    auto physicalDevices = _instance.enumeratePhysicalDevices().value;
    for (auto &physDev : physicalDevices) {
        DeviceCreateData devCreateData = CheckPhysicalDeviceSuitability(physDev, _settings);
        if (!devCreateData._physDevice)
            continue;
        vk::PhysicalDeviceProperties props = devCreateData._physDevice.getProperties();
        DeviceDescription deviceDesc{
            ._name = props.deviceName,
            ._vendorId = props.vendorID,
            ._deviceId = props.deviceID,
            ._driverVersion = props.driverVersion,
            ._isIntegrated = props.deviceType != vk::PhysicalDeviceType::eDiscreteGpu,
        };
        devices.push_back(deviceDesc);
    }
    return devices;
}

bool RhiVk::Init(Settings const &settings, int32_t deviceIndex)
{
    if (!Rhi::Init(settings))
        return false;

    if (!InitInstance())
        return false;

    if (!InitDevice(deviceIndex))
        return false;

    if (!_timelineSemaphore.Init(this, "RhiTimeline", 1))
        return false;

    if (!InitVma())
        return false;

    vk::PipelineCacheCreateInfo cacheInfo{};
    if (_device.createPipelineCache(&cacheInfo, AllocCallbacks(), &_pipelineCache) != vk::Result::eSuccess)
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

    vk::DebugUtilsMessengerCreateInfoEXT debugMessengerInfo{
        vk::DebugUtilsMessengerCreateFlagsEXT(),
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,// | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose,
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        DebugUtilsMessengerFunc,
        this
    };

    if (_settings._enableValidation) {
        instLayerNames.push_back("VK_LAYER_KHRONOS_validation");
        instExtNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    vk::InstanceCreateInfo inst{
        vk::InstanceCreateFlags(),
        &appInfo,
        instLayerNames,
        instExtNames,
        _settings._enableValidation ? &debugMessengerInfo : nullptr,
    };
    if (vk::createInstance(&inst, AllocCallbacks(), &_instance) != vk::Result::eSuccess)
        return false;

    _dynamicDispatch.init(_instance, vkGetInstanceProcAddr);

    if (_settings._enableValidation) {
        if (_instance.createDebugUtilsMessengerEXT(&debugMessengerInfo, AllocCallbacks(), &_debugUtilsMessenger, _dynamicDispatch) != vk::Result::eSuccess)
            return false;
    }

    return true;
}

bool RhiVk::InitDevice(int32_t deviceIndex)
{
    auto physicalDevices = _instance.enumeratePhysicalDevices().value;
    for (auto &physDev : physicalDevices) {
        DeviceCreateData devCreateData = CheckPhysicalDeviceSuitability(physDev, _settings);
        if (!devCreateData._physDevice)
            continue;
        if (deviceIndex-- > 0)
            continue;
        _physDevice = devCreateData._physDevice;

        std::array<float, 1> queuePriorities{ 1.0f };
        std::array<vk::DeviceQueueCreateInfo, 1> queueCreateInfo{ 
            vk::DeviceQueueCreateInfo {
                vk::DeviceQueueCreateFlags(),
                (uint32_t)devCreateData._universalQueueFamily,
                1,
                queuePriorities.data(),
            }
        };
        vk::PhysicalDeviceVulkan12Features features12;
        features12.setTimelineSemaphore(true);
        vk::PhysicalDeviceFeatures features;
        vk::DeviceCreateInfo devInfo{
            vk::DeviceCreateFlags(),
            queueCreateInfo,
            devCreateData._layerNames,
            devCreateData._extNames,
            &features,
            &features12
        };
        if (_physDevice.createDevice(&devInfo, AllocCallbacks(), &_device) != vk::Result::eSuccess)
            return false;

        _universalQueue._family = devCreateData._universalQueueFamily;
        _universalQueue._queue = _device.getQueue(devCreateData._universalQueueFamily, 0);
        ASSERT(_universalQueue._queue);
        if (!_universalQueue._queue)
            return false;

        SetDebugName(vk::ObjectType::eQueue, (uint64_t)(VkQueue)_universalQueue._queue, "UniversalQueue");

        return true;
    }
    return false;
}

bool RhiVk::InitVma()
{
    VmaAllocatorCreateInfo vmaInfo{
        .physicalDevice = _physDevice,
        .device = _device,
        .instance = _instance,
    };
    if ((vk::Result)vmaCreateAllocator(&vmaInfo, &_vma) != vk::Result::eSuccess)
        return false;
    return true;
}

bool RhiVk::SetDebugName(vk::ObjectType objType, uint64_t handle, char const *name)
{
    if (_settings._enableValidation) {
        vk::DebugUtilsObjectNameInfoEXT objName{
            objType,
            handle,
            name
        };
        if (_device.setDebugUtilsObjectNameEXT(objName, _dynamicDispatch) != vk::Result::eSuccess)
            return false;
    }
    return true;
}

std::span<uint32_t> RhiVk::GetQueueFamilyIndices(ResourceUsage usage)
{
    static std::array<uint32_t, 1> s_theQueue { { _universalQueue._family, } };
    return s_theQueue;
}

ResourceUsage RhiVk::GetFormatImageUsage(Format fmt, ResourceUsage usage)
{
    return GetUsageFromFormatFeatures(GetFormatFeatures(fmt, usage));
}

vk::FormatFeatureFlags RhiVk::GetFormatFeatures(Format fmt, ResourceUsage usage)
{
    vk::FormatProperties fmtProps = _physDevice.getFormatProperties(s_vk2Format.ToSrc(fmt, vk::Format::eUndefined));
    vk::ImageTiling tiling = GetImageTiling(usage);
    vk::FormatFeatureFlags fmtFlags = tiling == vk::ImageTiling::eLinear ? fmtProps.linearTilingFeatures : fmtProps.optimalTilingFeatures;
    return fmtFlags;
}

VmaAllocationCreateInfo RhiVk::GetVmaAllocCreateInfo(Resource *resource)
{
    VmaAllocationCreateInfo allocInfo{
        .usage = VMA_MEMORY_USAGE_AUTO,
    };
    if (resource->_descriptor._usage.cpuAccess) {
        allocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        if (resource->_descriptor._usage.copyDst)
            allocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    }
    if (_settings._enableValidation) {
        allocInfo.flags |= VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
        allocInfo.pUserData = const_cast<char*>(resource->_name.c_str());
    }

    return allocInfo;
}

bool RhiVk::WaitIdle()
{
    if (_device.waitIdle() != vk::Result::eSuccess)
        return false;
    return true;
}


}