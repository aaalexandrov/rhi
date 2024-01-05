#pragma once

#include "utl/geom_primitive.h"

namespace eng {

using utl::TypeInfo;

struct Object;
struct Component : public utl::Any {
	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Component>(); }

	Object *_parent = nullptr;

	virtual void Init(Object *parent);
};

struct World;
struct Object : public utl::Any {

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Object>(); }


	void *AddComponent(TypeInfo const *cmpType);
	template <typename T>
		requires std::is_base_of_v<Component, T>
	T *AddComponent() { return (T*)AddComponent(TypeInfo::Get<T>()); }

	void *GetComponent(TypeInfo const *cmpType);
	template <typename T>
		requires std::is_base_of_v<Component, T>
	T *GetComponent() { return (T *)GetComponent(TypeInfo::Get<T>()); }

	void RemoveComponent(Component *component);

	void SetWorld(World *world);

	World *GetWorld() const { return _world; }

	utl::Transform3F const &GetTransform() const { return _transform; }
	void SetTransform(utl::Transform3F const &transform);

	utl::GeomPrimitive3F const &GetLocalBound() const { return _localBound; }
	void SetLocalBound(utl::GeomPrimitive3F localBound);

	utl::BoxF GetBox() const { return GetBound().GetBoundingBox(); }
	utl::GeomPrimitive3F GetBound() const { return _localBound.GetTransformed(_transform); }

private:
	template <typename Fn>
	void UpdateWorldRegistration(Fn fnUpdate);

	World *_world = nullptr;
	utl::Transform3F _transform;
	utl::GeomPrimitive3F _localBound;
	std::vector<std::unique_ptr<Component>> _components;
};

}