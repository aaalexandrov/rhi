#define VMA_IMPLEMENTATION
#include "base_vk.h"
#include "rhi_vk.h"
#include "submit_vk.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("base_vk", [] {
    TypeInfo::Register<ResourceVk>().Name("ResourceVk");
});


CmdRecorderVk::~CmdRecorderVk()
{
    // buffers are freed by this call as well
    _rhi->_device.destroyCommandPool(_cmdPool, _rhi->AllocCallbacks());
}

bool CmdRecorderVk::Init(RhiVk *rhi, uint32_t queueFamily)
{
    _rhi = rhi;
    _queueFamily = queueFamily;
    vk::CommandPoolCreateInfo poolInfo{
        vk::CommandPoolCreateFlags(),
        _queueFamily,
    };
    if (_rhi->_device.createCommandPool(&poolInfo, _rhi->AllocCallbacks(), &_cmdPool) != vk::Result::eSuccess)
        return false;
    return true;
}

void CmdRecorderVk::Clear()
{
    _rhi->_device.freeCommandBuffers(_cmdPool, _cmdBuffers);
    _cmdBuffers.clear();
}

vk::CommandBuffer CmdRecorderVk::AllocCmdBuffer(vk::CommandBufferLevel level, std::string name)
{
    vk::CommandBufferAllocateInfo bufInfo{
        _cmdPool,
        level,
        1,
    };
    vk::CommandBuffer buffer;
    if (_rhi->_device.allocateCommandBuffers(&bufInfo, &buffer) != vk::Result::eSuccess)
        return vk::CommandBuffer();
    _cmdBuffers.push_back(buffer);
    return buffer;
}

vk::CommandBuffer CmdRecorderVk::BeginCmds(std::string name)
{
    vk::CommandBuffer cmds = AllocCmdBuffer(vk::CommandBufferLevel::ePrimary, name + std::to_string(_cmdBuffers.size()));
    vk::CommandBufferBeginInfo cmdBegin{
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
    };
    if (cmds.begin(cmdBegin) != vk::Result::eSuccess)
        return vk::CommandBuffer();

    return cmds;
}

bool CmdRecorderVk::EndCmds(vk::CommandBuffer cmds)
{
    if (cmds.end() != vk::Result::eSuccess)
        return false;

    return true;
}

bool CmdRecorderVk::Execute(Submission *sub)
{
    auto *subVk = static_cast<SubmissionVk *>(sub);

    ExecuteDataVk exec;
    exec._cmds.insert(exec._cmds.end(), _cmdBuffers.begin(), _cmdBuffers.end());
    if (!subVk->Execute(std::move(exec)))
        return false;

    return true;
}

bool TimelineSemaphoreVk::Init(RhiVk *rhi, uint64_t initValue)
{
    ASSERT(!_rhi);
    _rhi = rhi;
    vk::SemaphoreTypeCreateInfo semType{
        vk::SemaphoreType::eTimeline,
        0,
    };
    vk::SemaphoreCreateInfo semInfo{
        vk::SemaphoreCreateFlags(),
        &semType,
    };
    if (_rhi->_device.createSemaphore(&semInfo, _rhi->AllocCallbacks(), &_semaphore) != vk::Result::eSuccess)
        return false;

    return true;
}

void TimelineSemaphoreVk::Done()
{
    if (_rhi) {
        _rhi->_device.destroySemaphore(_semaphore, _rhi->AllocCallbacks());
        _semaphore = nullptr;
        _rhi = nullptr;
    }
}

uint64_t TimelineSemaphoreVk::GetCurrentCounter()
{
    auto result = _rhi->_device.getSemaphoreCounterValue(_semaphore);
    ASSERT(result.result == vk::Result::eSuccess);
    return result.result == vk::Result::eSuccess ? result.value : ~0ull;
}

bool TimelineSemaphoreVk::WaitCounter(uint64_t counter, uint64_t timeout)
{
    vk::SemaphoreWaitInfo waitInfo{
        vk::SemaphoreWaitFlags(),
        1,
        &_semaphore,
        &counter,
    };
    if (_rhi->_device.waitSemaphores(waitInfo, timeout) != vk::Result::eSuccess)
        return false;

    return true;
}

vk::PipelineStageFlags GetPipelineStages(ResourceUsage usage)
{
    vk::PipelineStageFlags flags;
    if (usage.rt || usage.ds)
        flags |= vk::PipelineStageFlagBits::eColorAttachmentOutput;
    if (usage.uav)
        flags |= vk::PipelineStageFlagBits::eComputeShader;
    if (usage.vb || usage.ib)
        flags |= vk::PipelineStageFlagBits::eVertexInput;
    if (usage.srv)
        flags |=
            vk::PipelineStageFlagBits::eVertexShader |
            vk::PipelineStageFlagBits::eFragmentShader |
            vk::PipelineStageFlagBits::eComputeShader;
    if (usage.copySrc | usage.copyDst | usage.present)
        flags |= vk::PipelineStageFlagBits::eTransfer;
    if (usage.cpuAccess)
        flags |= vk::PipelineStageFlagBits::eHost;

    if (!flags)
        flags |= vk::PipelineStageFlagBits::eTopOfPipe;

    return flags;
}

vk::AccessFlags GetAllAccess(ResourceUsage usage)
{
    vk::AccessFlags flags;
    if (usage.rt)
        flags |= vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
    if (usage.ds)
        flags |= vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    if (usage.ib)
        flags |= vk::AccessFlagBits::eIndexRead;
    if (usage.vb)
        flags |= vk::AccessFlagBits::eVertexAttributeRead;
    if (usage.srv)
        flags |= vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eUniformRead;
    if (usage.uav)
        flags |= vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite;
    if (usage.copySrc | usage.copyDst)
        flags |= vk::AccessFlagBits::eTransferRead | vk::AccessFlagBits::eTransferWrite;
    if (usage.cpuAccess)
        flags |= vk::AccessFlagBits::eHostRead | vk::AccessFlagBits::eHostWrite;

    return flags;
}

ResourceUsage GetUsageFromFormatFeatures(vk::FormatFeatureFlags fmtFlags)
{
    ResourceUsage usage;
    if (fmtFlags & vk::FormatFeatureFlagBits::eTransferSrc)
        usage.copySrc = 1;
    if (fmtFlags & vk::FormatFeatureFlagBits::eTransferDst)
        usage.copyDst = 1;
    if (fmtFlags & vk::FormatFeatureFlagBits::eSampledImage)
        usage.srv = 1;
    if (fmtFlags & vk::FormatFeatureFlagBits::eStorageImage)
        usage.uav = 1;
    if (fmtFlags & vk::FormatFeatureFlagBits::eColorAttachment)
        usage.rt = 1;
    if (fmtFlags & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
        usage.ds = 1;

    return usage;
}

vk::ImageAspectFlags GetImageAspect(Format fmt)
{
    vk::ImageAspectFlags flags;
    if (IsDepth(fmt))
        flags |= vk::ImageAspectFlagBits::eDepth;
    if (IsStencil(fmt))
        flags |= vk::ImageAspectFlagBits::eStencil;
    if (!flags)
        flags = vk::ImageAspectFlagBits::eColor;
    return flags;
}

vk::ImageSubresourceLayers GetImageSubresourceLayers(ResourceView const &view, uint32_t mipLevel)
{
    if (mipLevel == ~0u)
        mipLevel = view._mipRange._min;
    return vk::ImageSubresourceLayers{
        GetImageAspect(view._format),
        mipLevel,
        (uint32_t)view._region._min[3],
        std::max((uint32_t)view._region.GetSize()[3], 1u),
    };
}

vk::ImageTiling GetImageTiling(ResourceUsage usage)
{
    return usage.cpuAccess ? vk::ImageTiling::eLinear : vk::ImageTiling::eOptimal;
}

}