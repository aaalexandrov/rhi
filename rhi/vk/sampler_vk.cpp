#include "sampler_vk.h"
#include "rhi_vk.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("sampler_vk", [] {
	TypeInfo::Register<SamplerVk>().Name("SamplerVk")
		.Base<Sampler>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());
});


SamplerVk::~SamplerVk()
{
	auto rhi = static_cast<RhiVk*>(_rhi);
	rhi->_device.destroySampler(_sampler, rhi->AllocCallbacks());
}

bool SamplerVk::Init(SamplerDescriptor const &desc)
{
	if (!Sampler::Init(desc))
		return false;
	auto rhi = static_cast<RhiVk*>(_rhi);
	vk::SamplerCreateInfo samplerInfo{
		vk::SamplerCreateFlags(),
		s_vk2Filter.ToSrc(_descriptor._magFilter),
		s_vk2Filter.ToSrc(_descriptor._minFilter),
		s_vk2MipMapMode.ToSrc(_descriptor._mipMapMode),
		s_vk2AddressMode.ToSrc(_descriptor._addressModes[0]),
		s_vk2AddressMode.ToSrc(_descriptor._addressModes[1]),
		s_vk2AddressMode.ToSrc(_descriptor._addressModes[2]),
		_descriptor._mipLodBias,
		_descriptor._maxAnisotropy > 0,
		_descriptor._maxAnisotropy,
		_descriptor._compareOp != CompareOp::Always,
		s_vk2CompareOp.ToSrc(_descriptor._compareOp),
		_descriptor._minLod,
		_descriptor._maxLod,
	};
	if ((vk::Result)rhi->_device.createSampler(&samplerInfo, rhi->AllocCallbacks(), &_sampler) != vk::Result::eSuccess)
		return false;
	return true;
}

}