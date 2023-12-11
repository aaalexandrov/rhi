#include "texture_vk.h"
#include "rhi_vk.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("texture_vk", [] {
	TypeInfo::Register<TextureVk>().Name("TextureVk")
		.Base<Texture>()
		.Metadata(RhiOwned::s_rhiTagType, TypeInfo::Get<RhiVk>());
});


TextureVk::~TextureVk()
{
	auto rhi = static_pointer_cast<RhiVk>(_rhi.lock());
	vmaDestroyImage(rhi->_vma, _image, _vmaAlloc);
}

bool TextureVk::Init(ResourceDescriptor const &desc)
{
	return false;
}

}