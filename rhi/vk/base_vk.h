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

static inline utl::ValueRemapper<vk::Filter, Filter> s_vk2Filter{ {
		{ vk::Filter::eNearest , Filter::Nearest },
		{ vk::Filter::eLinear  , Filter::Linear  },
		{ vk::Filter::eCubicIMG, Filter::Cubic   },
	} };

static inline utl::ValueRemapper<vk::SamplerMipmapMode, MipMapMode>s_vk2MipMapMode{ {
		{ vk::SamplerMipmapMode::eNearest, MipMapMode::Nearest },
		{ vk::SamplerMipmapMode::eLinear , MipMapMode::Linear  },
	} };

static inline utl::ValueRemapper<vk::SamplerAddressMode, AddressMode> s_vk2AddressMode{ {
		{ vk::SamplerAddressMode::eRepeat        , AddressMode::Repeat         },
		{ vk::SamplerAddressMode::eClampToEdge   , AddressMode::ClampToEdge    },
		{ vk::SamplerAddressMode::eMirroredRepeat, AddressMode::MirroredRepeat },
	} };

static inline utl::ValueRemapper<vk::CompareOp, CompareOp> s_vk2CompareOp{ {
		{ vk::CompareOp::eNever         , CompareOp::Never          },
		{ vk::CompareOp::eLess          , CompareOp::Less           },
		{ vk::CompareOp::eEqual         , CompareOp::Equal          },
		{ vk::CompareOp::eLessOrEqual   , CompareOp::LessOrEqual    },
		{ vk::CompareOp::eGreater       , CompareOp::Greater        },
		{ vk::CompareOp::eNotEqual      , CompareOp::NotEqual       },
		{ vk::CompareOp::eGreaterOrEqual, CompareOp::GreaterOrEqual },
		{ vk::CompareOp::eAlways        , CompareOp::Always         },
	} };

vk::PipelineStageFlags GetPipelineStages(ResourceUsage usage);

static inline vk::AccessFlags s_accessReadFlags = 
	vk::AccessFlagBits::eIndirectCommandRead | 
	vk::AccessFlagBits::eIndexRead | 
	vk::AccessFlagBits::eVertexAttributeRead | 
	vk::AccessFlagBits::eUniformRead | 
	vk::AccessFlagBits::eInputAttachmentRead | 
	vk::AccessFlagBits::eShaderRead | 
	vk::AccessFlagBits::eColorAttachmentRead | 
	vk::AccessFlagBits::eDepthStencilAttachmentRead | 
	vk::AccessFlagBits::eTransferRead | 
	vk::AccessFlagBits::eHostRead | 
	vk::AccessFlagBits::eMemoryRead;

static inline vk::AccessFlags s_accessWriteFlags =
	vk::AccessFlagBits::eShaderWrite |
	vk::AccessFlagBits::eColorAttachmentWrite |
	vk::AccessFlagBits::eDepthStencilAttachmentWrite |
	vk::AccessFlagBits::eTransferWrite |
	vk::AccessFlagBits::eHostWrite |
	vk::AccessFlagBits::eMemoryWrite;

vk::AccessFlags GetAllAccess(ResourceUsage usage);
inline vk::AccessFlags GetReadAccess(ResourceUsage usage) { 
	return GetAllAccess(usage) & s_accessReadFlags; 
}
inline vk::AccessFlags GetWriteAccess(ResourceUsage usage) { 
	return GetAllAccess(usage) & s_accessWriteFlags; 
}

inline vk::Extent3D GetExtent3D(glm::vec3 dim) {
	return vk::Extent3D(dim[0], dim[1], dim[2]);
}


}