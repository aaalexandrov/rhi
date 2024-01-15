#include "component.h"
#include "render/renderer.h"
#include "rhi/pipeline.h"
#include "rhi/rhi.h"

namespace eng {

static auto s_regTypes = TypeInfo::AddInitializer("component", [] {
    TypeInfo::Register<CameraCmp>().Name("eng::CameraCmp")
        .Base<Component>();
    TypeInfo::Register<RenderingCmp>().Name("eng::RenderingCmp")
        .Base<Component>();
});


glm::mat3x4 CameraCmp::GetViewMatrix() const
{
    return _parent->GetTransform().Inverse().GetMatrix();
}

glm::mat4 CameraCmp::GetProjMatrix(glm::vec2 viewportSize) const
{
    return glm::perspective(_fovY, viewportSize.x / viewportSize.y, _near, _far);
}

utl::Polytope3F CameraCmp::GetFrustum(glm::vec2 viewportSize) const
{
    utl::BoxF postTransform(glm::vec3(-1, -1, 0), glm::vec3(1, 1, 1));
    glm::mat4 view = GetViewMatrix();
    glm::mat4 proj = GetProjMatrix(viewportSize);
    glm::mat4 viewProj = proj * view;
    // because transformed plane p' = transpose(inverse(M)) * p, for M = inverse(viewProj)
    glm::mat4 planePostTransformToWorld = transpose(viewProj);

    utl::Polytope3F frustum;

    using BoxSides = utl::BoxSides<utl::BoxF>;
    for (int s = 0; s < BoxSides::NumSides; ++s) {
        utl::PlaneF sidePostTransform = BoxSides::GetSide(postTransform, s);
        glm::vec4 planePostTransform(sidePostTransform._normal, sidePostTransform._d);
        glm::vec4 planeWorld = planePostTransformToWorld * planePostTransform;
        utl::PlaneF sideWorld(glm::vec3(planeWorld), planeWorld.w);
        frustum.AddSide(sideWorld);
    }

    frustum.UpdateAfterAddingSides();

    return frustum;
}

bool RenderingCmp::UpdateObjParams(RenderObjectsData &renderData)
{
    if (!_parent->IsTransformDirty())
        return true;

    int32_t transformsParam = _objParams->GetSetDescription()->GetParamIndex("objTransforms");
    if (transformsParam >= 0) {
        rhi::ResourceSetDescription::Param const &param = _objParams->GetSetDescription()->_params[transformsParam];
        ASSERT(param._kind == rhi::ShaderParam::UniformBuffer);
        rhi::ShaderParam const *shaderParam = _objParams->_pipeline->GetShaderParam(0, transformsParam);

        rhi::Rhi *rhi = _objParams->_pipeline->_rhi;

        rhi::ResourceDescriptor bufDesc{
            ._usage{.copySrc = 1, .cpuAccess = 1, },
            ._dimensions{ (int32_t)shaderParam->_type->_size },
        };
        auto uploadBuf = rhi->New<rhi::Buffer>("Update" + shaderParam->_name, bufDesc);
        auto uploadData = uploadBuf->Map();

        utl::AnyRef transforms{ shaderParam->_type, uploadData.data() };
        *transforms.GetMember("world").Get<glm::mat4>() = _parent->GetTransform().GetMatrix();

        uploadBuf->Unmap();
        auto uploadPass = rhi->Create<rhi::CopyPass>("Upload" + shaderParam->_name);
        uploadPass->Copy(rhi::CopyPass::CopyData{._src{uploadBuf}, ._dst{_objParams->_resourceRefs[transformsParam]._bindable}});
        renderData._updatePasses.push_back(std::move(uploadPass));

        _parent->SetTransformDirty(false);
    }

    return true;
}

}