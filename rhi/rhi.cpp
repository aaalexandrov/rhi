#include "rhi.h"
#include "pipeline.h"
#include "utl/file.h"

namespace rhi {

static auto s_regTypes = TypeInfo::AddInitializer("rhi", [] {
    TypeInfo::Register<Rhi>().Name("Rhi")
        .Base<utl::Any>();
});


Rhi::~Rhi()
{
}

auto Rhi::GetInitializedDevice() -> DeviceDescription
{
    return GetDevices(_settings).at(_deviceIndex);
}

bool Rhi::Init(Settings const &settings, int32_t deviceIndex)
{
    _settings = settings;
    if (!InitTypes())
        return false;

    _deviceIndex = deviceIndex;

    return true;
}

void Rhi::ClearCachedData()
{
    _pipelines.clear();
    _shaders.clear();
}

bool Rhi::InitTypes()
{
    bool res = true;
    TypeInfo::Get<RhiOwned>()->ForDerivedTypes([&](TypeInfo const *derived) {
        if (derived->GetMetadata(RhiOwned::s_rhiTagType))
            return Enum::Continue;
        TypeInfo const *toCreate = GetDerivedTypeWithTag(derived);
        ASSERT(toCreate);
        res = res && toCreate;
        return Enum::Continue;
    });

    return res;
}

std::shared_ptr<RhiOwned> Rhi::Create(TypeInfo const *type, std::string name)
{
    TypeInfo const *toCreate = GetDerivedTypeWithTag(type);
    ASSERT(toCreate);
    if (!toCreate)
        return std::shared_ptr<RhiOwned>();
    auto obj = std::shared_ptr<RhiOwned>(toCreate->ConstructAs<RhiOwned>(nullptr, true));
    if (!obj->InitRhi(this, name))
        obj = nullptr;
    return obj;
}

std::shared_ptr<Shader> Rhi::GetShader(std::string path, ShaderKind kind)
{
    ShaderData shaderData{
        ._name = utl::GetPathFilenameExt(path),
        ._kind = kind,
    };

    auto it = _shaders.find(shaderData);
    if (it == _shaders.end()) {
        auto shader = Create<Shader>();
        if (!shader->Load(shaderData, utl::ReadFile(path)))
            return nullptr;
        it = _shaders.insert({ shaderData, std::move(shader) }).first;
    }
    return it->second;
}

std::shared_ptr<Pipeline> Rhi::GetPipeline(PipelineData const &pipelineData, GraphicsPass *renderPass)
{
    PipelineData pipeData = pipelineData;
    pipeData.FillRenderTargetFormats(renderPass);

    auto it = _pipelines.find(pipeData);
    if (it == _pipelines.end()) {
        auto pipeline = New<Pipeline>("", pipeData, renderPass);
        if (!pipeline)
            return nullptr;
        it = _pipelines.insert({pipeData, std::move(pipeline)}).first;
    }
    return it->second;
}

std::shared_ptr<Submission> Rhi::Submit(std::vector<std::shared_ptr<Pass>> &&passes, std::string name)
{
    auto sub = Create<Submission>(name);
    sub->_passes = std::move(passes);
    return sub;
}

TypeInfo const *Rhi::GetDerivedTypeWithTag(TypeInfo const *base)
{
    {
        std::shared_lock rlock(_rwLock);
        auto it = _derivedTypes.find(base);
        if (it != _derivedTypes.end())
            return it->second;
    }
    TypeInfo const *rhiType = GetTypeInfo();
    auto typeHasTag = [=](TypeInfo const *type) {
        auto *tagType = type->GetMetadata<TypeInfo const *>(RhiOwned::s_rhiTagType);
        return tagType && *tagType == rhiType;
    };
    TypeInfo const *foundType = typeHasTag(base) ? base : nullptr;
    base->ForDerivedTypes([&](TypeInfo const *derived) {
        if (!typeHasTag(derived))
            return Enum::Continue;
        foundType = derived;
        return Enum::Stop;
    });
    {
        std::unique_lock wlock(_rwLock);
        _derivedTypes.insert({ base, foundType });
    }
    return foundType;
}

}