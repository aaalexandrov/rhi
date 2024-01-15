#pragma once

#include "utl/geom_primitive.h"

namespace eng {

using utl::TypeInfo;

struct Object;
struct Component : utl::Any {
	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Component>(); }

	Object *_parent = nullptr;

	virtual void Init(Object *parent);
};

struct World;
struct Object : std::enable_shared_from_this<Object>, utl::Any {

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Object>(); }


	void *AddComponent(TypeInfo const *cmpType);
	template <typename T>
		requires std::is_base_of_v<Component, T>
	T *AddComponent() { return (T*)AddComponent(TypeInfo::Get<T>()); }

	void *GetComponent(TypeInfo const *cmpType);
	void const *GetComponent(TypeInfo const *cmpType) const { return const_cast<Object *>(this)->GetComponent(cmpType); }
	template <typename T>
		requires std::is_base_of_v<Component, T>
	T *GetComponent() { return (T *)GetComponent(TypeInfo::Get<T>()); }
	template <typename T>
	T const *GetComponent() const { return const_cast<Object*>(this)->GetComponent<T>(); }

	void RemoveComponent(Component *component);

	void SetWorld(World *world);

	World *GetWorld() const { return _world; }

	utl::Transform3F const &GetTransform() const { return _transform; }
	void SetTransform(utl::Transform3F const &transform);

	bool IsTransformDirty() const { return _transformDirty; }
	void SetTransformDirty(bool dirty) { _transformDirty = dirty; }

	utl::GeomPrimitive3F const &GetLocalBound() const { return _localBound; }
	void SetLocalBound(utl::GeomPrimitive3F localBound);

	utl::BoxF GetBox() const { return GetBound().GetBoundingBox(); }
	utl::GeomPrimitive3F GetBound() const { return _localBound.GetTransformed(_transform); }

private:
	template <typename Fn>
	void UpdateWorldRegistration(Fn fnUpdate);

	World *_world = nullptr;
	utl::Transform3F _transform;
	bool _transformDirty = true;
	utl::GeomPrimitive3F _localBound{glm::vec3(0)};
	std::vector<std::unique_ptr<Component>> _components;
};

}