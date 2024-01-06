#pragma once

#include "utl/box_tree.h"

namespace eng {

using utl::TypeInfo;

struct Object;
struct World : utl::Any {

	void Init(utl::BoxF const &worldBox, glm::vec3 minWorldNodeSize);

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<World>(); }

	void Update(Object *obj, utl::BoxF const &curBox, utl::BoxF const &newBox);

	using ObjectTree = utl::BoxTree<glm::vec3, std::unordered_set<std::shared_ptr<Object>>>;
	
	ObjectTree _objTree;
};

}