#pragma once

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_NO_SMART_HANDLE
//#define VULKAN_HPP_ASSERT ASSERT
#if !defined(NDEBUG)
#define VULKAN_HPP_ASSERT(expr) (void)sizeof(expr)
#endif
#if defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__linux__)
#define VK_USE_PLATFORM_XLIB_KHR
#define VK_USE_PLATFORM_WAYLAND_KHR
#endif

#include "vulkan/vulkan.hpp"

#if defined(min)
#undef min
#endif
#if defined(max)
#undef max
#endif

#if defined(Always)
#undef Always
#endif

#if defined(None)
#undef None
#endif

#include "vk_mem_alloc.h"

#include "../base.h"
#include "utl/enumutl.h"

namespace rhi {

struct RhiVk;
struct Submission;
struct CmdRecorderVk {
	~CmdRecorderVk();

	bool Init(RhiVk *rhi, uint32_t queueFamily);

	void Clear();

	vk::CommandBuffer AllocCmdBuffer(vk::CommandBufferLevel level, std::string name);

	vk::CommandBuffer BeginCmds(std::string name);
	bool EndCmds(vk::CommandBuffer cmds);

	bool Execute(Submission *sub);

	RhiVk *_rhi = nullptr;
	uint32_t _queueFamily = ~0;
	std::vector<vk::CommandBuffer> _cmdBuffers;
	vk::CommandPool _cmdPool;
};

struct TimelineSemaphoreVk {
	~TimelineSemaphoreVk() {
		Done();
	}
	bool Init(RhiVk *rhi, std::string name, uint64_t initValue = 0);
	void Done();

	uint64_t GetCurrentCounter();
	bool WaitCounter(uint64_t counter, uint64_t timeout = std::numeric_limits<uint64_t>::max());

	RhiVk *_rhi = nullptr;
	vk::Semaphore _semaphore;
	std::atomic<uint64_t> _value;
	std::string _name;
};

struct SemaphoreReferenceVk {
	vk::Semaphore _semaphore;
	vk::PipelineStageFlags _stages;
	uint64_t _counter = ~0ull;
};

struct ResourceStateVk {
	vk::AccessFlags _access;
	vk::PipelineStageFlags _stages;
	vk::ImageLayout _layout;
	SemaphoreReferenceVk _semaphore;
};

struct ResourceTransitionVk {
	ResourceStateVk _srcState, _dstState;
	bool HasLayouts() const {
		return 
			_srcState._layout != vk::ImageLayout::eUndefined || 
			_dstState._layout != vk::ImageLayout::eUndefined;
	}
};

struct ResourceVk {
	virtual ~ResourceVk() {}
	virtual ResourceTransitionVk GetTransitionData(ResourceUsage prevUsage, ResourceUsage usage) = 0;
};

vk::PipelineStageFlags GetPipelineStages(ResourceUsage usage);
vk::ImageAspectFlags GetImageAspect(Format fmt);
vk::ImageSubresourceLayers GetImageSubresourceLayers(ResourceView const &view, uint32_t mipLevel = ~0u);
vk::ImageTiling GetImageTiling(ResourceUsage usage);


static inline constexpr vk::AccessFlags s_accessReadFlags = 
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

static inline constexpr vk::AccessFlags s_accessWriteFlags =
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
inline vk::AccessFlags GetAccess(ResourceUsage usage) {
	ASSERT(usage.read || usage.write || usage.create);
	vk::AccessFlags access;
	if (usage.read) {
		if (usage.write)
			access = GetAllAccess(usage);
		else 
			access = GetReadAccess(usage);
	} else if (usage.write)
		access = GetWriteAccess(usage);
	return access;
}

inline vk::Extent2D GetExtent2D(glm::ivec2 dim) {
	dim = glm::max(dim, glm::ivec2(1));
	return vk::Extent2D(dim.x, dim.y);
}

inline vk::Offset2D GetOffset2D(glm::ivec2 v) {
	return vk::Offset2D(v.x, v.y);
}

inline vk::Extent3D GetExtent3D(glm::ivec3 dim) {
	dim = glm::max(dim, glm::ivec3(1));
	return vk::Extent3D(dim.x, dim.y, dim.z);
}

inline vk::Offset3D GetOffset3D(glm::ivec3 v) {
	return vk::Offset3D(v.x, v.y, v.z);
}

inline vk::Viewport GetViewport(utl::BoxF const &vp) {
	glm::vec3 vpSize = vp.GetSize();
	return vk::Viewport{vp._min.x, vp._min.y, vpSize.x, vpSize.y, vp._min.z, vp._max.z};
}

ResourceUsage GetUsageFromFormatFeatures(vk::FormatFeatureFlags fmtFlags);

static inline const utl::ValueRemapper<vk::Format, Format> s_vk2Format{ {
		{ vk::Format::eUndefined,          Format::Invalid       },
		{ vk::Format::eR8G8B8A8Unorm,      Format::R8G8B8A8      },
		{ vk::Format::eR8G8B8A8Srgb,       Format::R8G8B8A8_srgb },
		{ vk::Format::eB8G8R8A8Unorm,      Format::B8G8R8A8      },
		{ vk::Format::eB8G8R8A8Srgb,       Format::B8G8R8A8_srgb },
		{ vk::Format::eR8Unorm,            Format::R8            },
		{ vk::Format::eR8G8Unorm,          Format::R8G8          },
		{ vk::Format::eR8G8B8Unorm,        Format::R8G8B8        },
		{ vk::Format::eR16Uint,            Format::R16_u         },
		{ vk::Format::eR32Uint,            Format::R32_u         },
		{ vk::Format::eD24UnormS8Uint,     Format::D24S8         },
		{ vk::Format::eD32SfloatS8Uint,    Format::D32S8         },
		{ vk::Format::eD32Sfloat,          Format::D32           },
		{ vk::Format::eS8Uint,             Format::S8            },
	} };

static inline const utl::ValueRemapper<VkImageUsageFlags, ResourceUsage, true> s_vkImageUsage2ResourceUsage{ {
		{ (VkImageUsageFlags)vk::ImageUsageFlagBits::eTransferSrc           , ResourceUsage{.copySrc = 1} },
		{ (VkImageUsageFlags)vk::ImageUsageFlagBits::eTransferDst           , ResourceUsage{.copyDst = 1} },
		{ (VkImageUsageFlags)vk::ImageUsageFlagBits::eSampled               , ResourceUsage{.srv = 1}     },
		{ (VkImageUsageFlags)vk::ImageUsageFlagBits::eStorage               , ResourceUsage{.uav = 1}     },
		{ (VkImageUsageFlags)vk::ImageUsageFlagBits::eColorAttachment       , ResourceUsage{.rt = 1}      },
		{ (VkImageUsageFlags)vk::ImageUsageFlagBits::eDepthStencilAttachment, ResourceUsage{.ds = 1}      },
	} };

static inline const utl::ValueRemapper<vk::PresentModeKHR, PresentMode> s_vk2PresentMode{ {
		{vk::PresentModeKHR::eImmediate  , PresentMode::Immediate },
		{vk::PresentModeKHR::eMailbox    , PresentMode::Mailbox },
		{vk::PresentModeKHR::eFifo       , PresentMode::Fifo },
		{vk::PresentModeKHR::eFifoRelaxed, PresentMode::FifoRelaxed },
	} };

static inline const utl::ValueRemapper<vk::Filter, Filter> s_vk2Filter{ {
		{ vk::Filter::eNearest , Filter::Nearest },
		{ vk::Filter::eLinear  , Filter::Linear  },
		{ vk::Filter::eCubicIMG, Filter::Cubic   },
	} };

static inline const utl::ValueRemapper<vk::SamplerMipmapMode, MipMapMode>s_vk2MipMapMode{ {
		{ vk::SamplerMipmapMode::eNearest, MipMapMode::Nearest },
		{ vk::SamplerMipmapMode::eLinear , MipMapMode::Linear  },
	} };

static inline const utl::ValueRemapper<vk::SamplerAddressMode, AddressMode> s_vk2AddressMode{ {
		{ vk::SamplerAddressMode::eRepeat        , AddressMode::Repeat         },
		{ vk::SamplerAddressMode::eClampToEdge   , AddressMode::ClampToEdge    },
		{ vk::SamplerAddressMode::eMirroredRepeat, AddressMode::MirroredRepeat },
	} };

static inline const utl::ValueRemapper<vk::CompareOp, CompareOp> s_vk2CompareOp{ {
		{ vk::CompareOp::eNever         , CompareOp::Never          },
		{ vk::CompareOp::eLess          , CompareOp::Less           },
		{ vk::CompareOp::eEqual         , CompareOp::Equal          },
		{ vk::CompareOp::eLessOrEqual   , CompareOp::LessOrEqual    },
		{ vk::CompareOp::eGreater       , CompareOp::Greater        },
		{ vk::CompareOp::eNotEqual      , CompareOp::NotEqual       },
		{ vk::CompareOp::eGreaterOrEqual, CompareOp::GreaterOrEqual },
		{ vk::CompareOp::eAlways        , CompareOp::Always         },
	} };

}