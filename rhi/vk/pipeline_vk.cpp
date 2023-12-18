#include "pipeline_vk.h"
#include "rhi_vk.h"

#include "shaderc/shaderc.hpp"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("pipeline_vk", [] {
	TypeInfo::Register<ShaderVk>().Name("ShaderVk")
		.Base<Shader>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());

	TypeInfo::Register<PipelineVk>().Name("PipelineVk")
		.Base<Pipeline>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());
});


ShaderVk::~ShaderVk()
{
	auto rhi = static_cast<RhiVk *>(_rhi);
	rhi->_device.destroyShaderModule(_shaderModule, rhi->AllocCallbacks());
}

bool ShaderVk::Load(std::string name, ShaderKind kind, std::vector<uint8_t> const &content)
{
	auto rhi = static_cast<RhiVk *>(_rhi);

	ASSERT(content.size() % sizeof(uint32_t) == 0);
	vk::ShaderModuleCreateInfo modInfo{
		vk::ShaderModuleCreateFlags(),
		content.size(),
		(uint32_t *)content.data(),
	};
	if (rhi->_device.createShaderModule(&modInfo, rhi->AllocCallbacks(), &_shaderModule) != vk::Result::eSuccess)
		return false;

	return true;
}

}