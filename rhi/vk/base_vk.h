#pragma once

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_NO_SMART_HANDLE
#if defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__linux__)
#define VK_USE_PLATFORM_XLIB_KHR
#endif

#include "vulkan/vulkan.hpp"

#if defined(min)
#undef min
#endif
#if defined(max)
#undef max
#endif

#include "../base.h"

namespace rhi {


}