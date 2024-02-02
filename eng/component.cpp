#include "component.h"
#include "render/scene.h"
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
    glm::mat4 viewProjInv = inverse(viewProj);

    utl::Polytope3F frustum;

    using BoxSides = utl::BoxSides<utl::BoxF>;
    for (int s = 0; s < BoxSides::NumSides; ++s) {
        utl::PlaneF sidePostTransform = BoxSides::GetSide(postTransform, s);

        glm::vec3 sidePoint = sidePostTransform.GetAnyPoint();
        glm::vec3 worldPoint = utl::VecAffine(viewProjInv * glm::vec4(sidePoint, 1));
        glm::vec3 worldPoint1 = utl::VecAffine(viewProjInv * glm::vec4(sidePoint + sidePostTransform._normal, 1));
        glm::vec3 worldNormal = normalize(worldPoint1 - worldPoint);
        utl::PlaneF sideWorld = utl::PlaneF(worldNormal, worldPoint);

        frustum.AddSide(sideWorld);
    }

    frustum.UpdateAfterAddingSides();

    return frustum;
}

bool RenderingCmp::UpdateObjParams(RenderObjectsData &renderData)
{
    if (!_parent->IsTransformDirty() || _models.empty())
        return true;

    if (!_objParams) {
        _objParams = Scene::CreateResourseSetWithBuffer(_models[0]._pipeline.get(), 0, "ModelData");
    }

    auto uploadPass = Scene::UpdateResourceSetBuffer(_objParams.get(), "ModelData", [this](utl::AnyRef params) {
        *params.GetMember("world").Get<glm::mat4>() = _parent->GetTransform().GetMatrix();
        return true;
    });

    _parent->SetTransformDirty(false);
    renderData._updatePasses.push_back(std::move(uploadPass));

    return true;
}

}