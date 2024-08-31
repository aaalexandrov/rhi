#include "sys.h"
#include "world.h"
#include "render/scene.h"
#include "rhi/vk/rhi_vk.h"

#include "stb_image.h"

namespace eng {
Sys::~Sys()
{
}

bool Sys::Init()
{
    _ui = std::make_unique<Ui>();
    if (!_ui->Init())
        return false;

    return true;
}

bool Sys::InitRhi(std::shared_ptr<Window> const &window, int32_t deviceIndex)
{
    _rhi = std::static_pointer_cast<rhi::Rhi>(std::make_shared<rhi::RhiVk>());

    rhi::Rhi::Settings rhiSettings{
        ._appName = window->_desc._name.c_str(),
        ._appVersion = glm::uvec3(0, 1, 0),
#if !defined(NDEBUG)
        ._enableValidation = true,
#endif
        ._window = window->GetWindowData(),
    };

    if (!_rhi->Init(rhiSettings, deviceIndex))
        return false;

    LOG("Created rhi device '%s'", _rhi->GetInitializedDevice()._name);

    for (auto *win : _ui->_windows) {
        ASSERT(!win->_swapchain);
        if (!win->InitRendering())
            return false;
    }

    return true;
}

std::shared_ptr<rhi::Texture> Sys::LoadTexture(std::string path, bool genMips)
{
	auto rhi = eng::Sys::Get()->_rhi.get();
	int x, y, ch;
	uint8_t *img = stbi_load(path.c_str(), &x, &y, &ch, 0);

	if (!img)
		return nullptr;

	rhi::Format fmt;
	switch (ch) {
		case 1:
			fmt = rhi::Format::R8;
			break;
		case 2:
			fmt = rhi::Format::R8G8;
			break;
		case 4:
			fmt = rhi::Format::R8G8B8A8;
			break;
		default:
			return nullptr;
	}
	glm::vec4 dims{ x, y, 0, 0 };
	rhi::ResourceDescriptor texDesc{
		._usage{.srv = 1, .copySrc = genMips, .copyDst = 1},
		._format{fmt},
		._dimensions{dims},
		._mipLevels{rhi::GetMaxMipLevels(dims)},
	};
	std::shared_ptr<rhi::Texture> tex = rhi->New<rhi::Texture>(path, texDesc);

	std::shared_ptr<rhi::Buffer> staging = rhi->New<rhi::Buffer>("Staging " + path, rhi::ResourceDescriptor{
		._usage{.copySrc = 1, .cpuAccess = 1},
		._dimensions{x * y * ch, 0, 0, 0},
		});
	std::span<uint8_t> pixMem = staging->Map();
	memcpy(pixMem.data(), img, pixMem.size());
	staging->Unmap();

	stbi_image_free(img);

	std::vector<std::shared_ptr<rhi::Pass>> passes;
	auto copyStaging = rhi->Create<rhi::CopyPass>();
	copyStaging->Copy(rhi::CopyPass::CopyData{ ._src{staging}, ._dst{tex, rhi::ResourceView{._mipRange{0, 0}}} });
	passes.push_back(copyStaging);

	if (genMips) {
		auto mipGen = rhi->Create<rhi::CopyPass>();
		mipGen->CopyTopToLowerMips(tex);
		passes.push_back(mipGen);
	}

	auto sub = rhi->Submit(std::move(passes), "Upload " + path);
	sub->Prepare();
	sub->Execute();
	sub->WaitUntilFinished();

	return tex;
}


void Sys::UpdateTime(utl::UpdateQueue::Time deltaSec)
{
	utl::UpdateQueue::Time delta = deltaSec * _timeScale;
    _updateQueue.UpdateToTime(delta);

    if (_world)
        _world->UpdateTime(delta);
}


bool Sys::InitInstance()
{
    ASSERT(!s_instance);
    s_instance = std::make_unique<Sys>();
    if (!s_instance->Init())
        return false;
    return true;
}

void Sys::DoneInstance()
{
    s_instance = nullptr;
}

}