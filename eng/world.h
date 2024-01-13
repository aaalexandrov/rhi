#pragma once

#include "utl/box_tree.h"
#include "utl/geom_primitive.h"

namespace eng {

using utl::TypeInfo;

struct Object;
struct CameraCmp;
struct World : utl::Any {

	void Init(utl::BoxF const &worldBox, glm::vec3 minWorldNodeSize);

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<World>(); }

	void Update(Object *obj, utl::BoxF const &curBox, utl::BoxF const &newBox);

	using ObjEnumFn = std::function<utl::Enum(std::shared_ptr<Object> const &obj)>;
	template <typename Shape>
	utl::Enum EnumObjects(Shape const &volume, ObjEnumFn objEnumFn);

	utl::Enum EnumVisibleObjects(std::shared_ptr<CameraCmp> const &camera, glm::vec2 viewportSize, ObjEnumFn objEnumFn);

	using ObjectTree = utl::BoxTree<glm::vec3, std::unordered_set<std::shared_ptr<Object>>>;
	
	ObjectTree _objTree;
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
			utl::Enum res = objEnumFn(obj);
			if (res != utl::Enum::Continue)
				return res;
		}
		return utl::Enum::Continue;
	});
}

}