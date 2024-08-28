#include "component.h"
#include "render/scene.h"
#include "rhi/pipeline.h"
#include "rhi/rhi.h"
#include "sys.h"
#include "ui/ui.h"

namespace eng {

static auto s_regTypes = TypeInfo::AddInitializer("component", [] {
    TypeInfo::Register<CameraCmp>().Name("eng::CameraCmp")
        .Base<Component>();
    TypeInfo::Register<RenderingCmp>().Name("eng::RenderingCmp")
        .Base<Component>();
});


utl::UpdateQueue::Time CameraCmp::Update(utl::UpdateQueue *queue, utl::UpdateQueue::Time time)
{
    if (!Sys::Get()->_ui->_keyboardFocusInUI && this == Sys::Get()->_scene->_camera) {
        utl::UpdateQueue::Time deltaTime = time - queue->_lastUpdateTime;
        utl::Transform3F xform = eng::TransformFromKeyboardInput(deltaTime);
        _parent->SetTransform(_parent->GetTransform() * xform);
    }

    return 0;
}

glm::mat4x3 CameraCmp::GetViewMatrix() const
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

    glm::vec3 centerWorld = utl::VecAffine(viewProjInv * glm::vec4(postTransform.GetCenter(), 1));
    for (int d = 0; d < 3; ++d) {
        for (int e = 0; e < 2; ++e) {
            std::array<glm::vec3, 4> points;
            glm::vec3 weight{ 0 };
            weight[d] = e;
            for (int e1 = 0; e1 < 2; ++e1) {
                weight[(d + 1) % 3] = e1;
                for (int e2 = 0; e2 < 2; ++e2) {
                    weight[(d + 2) % 3] = e2;
                    points[e1 * 2 + e2] = utl::VecAffine(viewProjInv * glm::vec4(postTransform.GetPoint(weight), 1));
                }
            }
            utl::PlaneF sideWorld = utl::PlaneF::GetPlane(points).Normalized();
            if (sideWorld.Eval(centerWorld) > 0)
                sideWorld = sideWorld.Inverted();
            ASSERT(sideWorld.Eval(centerWorld) < 0);
            frustum.AddSide(sideWorld);
        }
    }

    frustum.UpdateAfterAddingSides(1e-3f);

    return frustum;
}

void RenderingCmp::UpdateObjectBoundFromModels()
{
    utl::BoxF box;
    for (auto &model : _models)
        box = box.GetUnion(model._mesh->_bound.GetBoundingBox());
    if (box.IsEmpty())
        _parent->SetLocalBound(utl::GeomPrimitive3F(glm::vec3(0)));
    else
        _parent->SetLocalBound(utl::GeomPrimitive3F(box));
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