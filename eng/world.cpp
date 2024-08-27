#include "world.h"
#include "object.h"
#include "component.h"

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