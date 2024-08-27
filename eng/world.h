#pragma once

#include "utl/box_tree.h"
#include "utl/geom_primitive.h"
#include "utl/update_queue.h"
#include "object.h"

namespace eng {

using utl::TypeInfo;

struct CameraCmp;
struct World : utl::Any {

	void Init(std::string name, utl::BoxF const &worldBox, glm::vec3 minWorldNodeSize);

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<World>(); }

	void Update(Object *obj, utl::BoxF const &curBox, utl::BoxF const &newBox);

	using ObjEnumFn = std::function<utl::Enum(std::shared_ptr<Object> &obj)>;
	template <typename Shape>
	utl::Enum EnumObjects(Shape const &volume, ObjEnumFn objEnumFn);

	utl::Enum EnumObjects(ObjEnumFn objEnumFn);

	void UpdateTime(utl::UpdateQueue::Time deltaTime);

	using ObjectTree = utl::BoxTree<glm::vec3, std::unordered_set<std::shared_ptr<Object>>>;
	
	std::string _name = "World";
	ObjectTree _objTree;
	utl::UpdateQueue _updateQueue;
	utl::UpdateQueue::Time _timeScale = 1.0;
};

template<typename Shape>
inline utl::Enum World::EnumObjects(Shape const &volume, ObjEnumFn objEnumFn)
{
	return _objTree.EnumNodes([&](ObjectTree::Node *node, uint32_t nodeIdx, utl::BoxF const &nodeBox) {
		if (!volume.Intersects(nodeBox))
			return utl::Enum::SkipChildren;
		for (auto &obj : node->_data) {
			utl::GeomPrimitive3F objBound = obj->GetBound();
			if (!objBound.Intersects(volume)) 
				continue;
			utl::Enum res = objEnumFn(const_cast<std::shared_ptr<Object>&>(obj));
			if (res != utl::Enum::Continue)
				return res;
		}
		return utl::Enum::Continue;
	});
}

}