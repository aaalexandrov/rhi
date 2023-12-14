#define VMA_IMPLEMENTATION
#include "base_vk.h"

namespace rhi {

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