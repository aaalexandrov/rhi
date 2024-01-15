#include "object.h"
#include "world.h"

namespace eng {

static auto s_regTypes = TypeInfo::AddInitializer("object", [] {
    TypeInfo::Register<Component>().Name("eng::Component")
        .Base<utl::Any>();
    TypeInfo::Register<Object>().Name("eng::Object")
        .Base<utl::Any>();
});


void Component::Init(Object *parent)
{
    ASSERT(!_parent);
    _parent = parent;
}


void *Object::AddComponent(TypeInfo const *cmpType)
{
    ASSERT(!GetComponent(cmpType));
    Component *cmp = _components.emplace_back(cmpType->ConstructAs<Component>(nullptr, true)).get();
    cmp->Init(this);
    return cmp;
}

void *Object::GetComponent(TypeInfo const *cmpType)
{
    for (auto &cmp : _components) {
        if (cmp->IsKindOf(cmpType))
            return cmp.get();
    }
    return nullptr;
}

void Object::RemoveComponent(Component *component)
{
    auto it = std::find_if(_components.begin(), _components.end(), [=](auto &cmp) { return cmp.get() == component; });
    ASSERT(it != _components.end());
    _components.erase(it);
}

void Object::SetWorld(World *world)
{
    ASSERT(bool(world) == !_world);

    if (_world) {
        _world->Update(this, GetBox(), utl::BoxF());
    }

    _world = world;

    if (_world) {
        _world->Update(this, utl::BoxF(), GetBox());
    }
}

template<typename Fn>
inline void Object::UpdateWorldRegistration(Fn fnUpdate)
{
    utl::BoxF prevBox = GetBox();
    fnUpdate();
    utl::BoxF newBox = GetBox();
    if (_world)
        _world->Update(this, prevBox, newBox);
}


void Object::SetTransform(utl::Transform3F const &transform)
{
    UpdateWorldRegistration([&] {
        _transform = transform;
        _transformDirty = true;
    });
}

void Object::SetLocalBound(utl::GeomPrimitive3F localBound)
{
    UpdateWorldRegistration([&] {
        _localBound = localBound;
    });
}

}