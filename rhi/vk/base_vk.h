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

#include "vma/vk_mem_alloc.h"

#include "../base.h"
#include "utl/enumutl.h"

namespace rhi {

static inline utl::ValueRemapper<vk::Format, Format> s_vk2Format{ {
		{ vk::Format::eUndefined,          Format::Invalid       },
		{ vk::Format::eR8G8B8A8Unorm,      Format::R8G8B8A8      },
		{ vk::Format::eR8G8B8A8Srgb,       Format::R8G8B8A8_srgb },
		{ vk::Format::eB8G8R8A8Unorm,      Format::B8G8R8A8      },
		{ vk::Format::eB8G8R8A8Srgb,       Format::B8G8R8A8_srgb },
		{ vk::Format::eR8Unorm,            Format::R8            },
		{ vk::Format::eD24UnormS8Uint,     Format::D24S8         },
		{ vk::Format::eD32SfloatS8Uint,    Format::D32S8         },
		{ vk::Format::eD32Sfloat,          Format::D32           },
		{ vk::Format::eS8Uint,             Format::S8            },
	} };


static inline utl::ValueRemapper<vk::PresentModeKHR, PresentMode> s_vk2PresentMode{ {
		{vk::PresentModeKHR::eImmediate  , PresentMode::Immediate },
		{vk::PresentModeKHR::eMailbox    , PresentMode::Mailbox },
		{vk::PresentModeKHR::eFifo       , PresentMode::Fifo },
		{vk::PresentModeKHR::eFifoRelaxed, PresentMode::FifoRelaxed },
	} };


}