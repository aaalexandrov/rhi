#define VMA_IMPLEMENTATION
#include "base_vk.h"
#include "rhi_vk.h"

namespace rhi {

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
        vk::PipelineStageFlagBits::eTessellationControlShader |
        vk::PipelineStageFlagBits::eTessellationEvaluationShader |
        vk::PipelineStageFlagBits::eGeometryShader |
        vk::PipelineStageFlagBits::eFragmentShader |
        vk::PipelineStageFlagBits::eComputeShader;
    if (usage.copySrc | usage.copyDst)
        flags |= vk::PipelineStageFlagBits::eTransfer;
    if (usage.cpuAccess)
        flags |= vk::PipelineStageFlagBits::eHost;

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

    return vk::AccessFlags();
}

}