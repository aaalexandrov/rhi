#include "component.h"

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

}