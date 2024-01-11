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

}