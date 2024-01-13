#include "world.h"
#include "object.h"
#include "component.h"

namespace eng {

static auto s_regTypes = TypeInfo::AddInitializer("world", [] {
    TypeInfo::Register<World>().Name("eng::World")
        .Base<utl::Any>();
});

void World::Init(utl::BoxF const &worldBox, glm::vec3 minWorldNodeSize)
{
    _objTree.Init(worldBox, minWorldNodeSize);
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

utl::Enum World::EnumVisibleObjects(std::shared_ptr<CameraCmp> const &camera, glm::vec2 viewportSize, ObjEnumFn objEnumFn)
{
    utl::Polytope3F frustum = camera->GetFrustum(viewportSize);
    return EnumObjects(frustum, objEnumFn);
}

}