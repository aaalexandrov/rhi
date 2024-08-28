#include "world.h"
#include "object.h"
#include "component.h"
#include "render/scene.h"

namespace eng {

static auto s_regTypes = TypeInfo::AddInitializer("world", [] {
    TypeInfo::Register<World>().Name("eng::World")
        .Base<utl::Any>();
});

void World::Init(std::string name, utl::BoxF const &worldBox, glm::vec3 minWorldNodeSize)
{
    _name = name;
    _objTree.Init(worldBox, minWorldNodeSize);
}

std::shared_ptr<Object> World::CreateCamera(std::string_view name)
{
    auto camera = std::make_shared<eng::Object>();
    camera->_name = name;
    camera->AddComponent<eng::CameraCmp>();
    camera->SetTransform(utl::Transform3F(glm::vec3(0, 0, 2), glm::angleAxis(0 * glm::pi<float>(), glm::vec3(0, 1, 0)), 1.0f));
    camera->SetWorld(this);
    return camera;
}

std::unique_ptr<Scene> World::CreateScene()
{
    CameraCmp *camera = nullptr;
    EnumObjects([&](std::shared_ptr<eng::Object> &obj) {
        camera = obj->GetComponent<eng::CameraCmp>();
        return camera ? utl::Enum::Stop : utl::Enum::Continue;
    });

    std::array<rhi::RenderTargetData, 1> renderTargets{
        {
            std::shared_ptr<rhi::Texture>(),
            glm::vec4(0, 0, 1, 1),
        },
    };

    return std::make_unique<eng::Scene>(this, camera, renderTargets);
}

void World::Update(Object *obj, utl::BoxF const &curBox, utl::BoxF const &newBox)
{
    uint32_t curNodeInd = _objTree.GetNodeIndex(curBox);
    uint32_t newNodeInd = _objTree.GetNodeIndex(newBox);
    if (curNodeInd == newNodeInd)
        return;
    
    auto objPtr = obj->shared_from_this();
    ObjectTree::Node *curNode = _objTree.GetNode(curNodeInd);
    ObjectTree::Node *newNode = _objTree.GetNode(newNodeInd, true);
    if (curNode) {
        curNode->_data.erase(objPtr);
        if (curNode->_data.empty() && curNode->_childMask == 0)
            _objTree.RemoveNode(curNodeInd);
    }
    if (newNode)
        newNode->_data.insert(std::move(objPtr));
}

utl::Enum World::EnumObjects(ObjEnumFn objEnumFn)
{
    return _objTree.EnumNodes([&](ObjectTree::Node *node, uint32_t nodeIdx, utl::BoxF const &nodeBox) {
        for (auto &obj : node->_data) {
            utl::Enum res = objEnumFn(const_cast<std::shared_ptr<Object>&>(obj));
            if (res != utl::Enum::Continue)
                return res;
        }
        return utl::Enum::Continue;
    });
}

void World::UpdateTime(utl::UpdateQueue::Time deltaTime)
{
    _updateQueue.UpdateToTime(deltaTime * _timeScale);
}

}