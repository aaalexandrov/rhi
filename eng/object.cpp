#include "object.h"
#include "world.h"
#include "sys.h"

namespace eng {

static auto s_regTypes = TypeInfo::AddInitializer("object", [] {
    TypeInfo::Register<Component>().Name("eng::Component")
        .Base<utl::Any>();
    TypeInfo::Register<Object>().Name("eng::Object")
        .Base<utl::Any>();
});


Component::~Component()
{
    if (_parent)
        SetScheduled(false);
}

void Component::Init(Object *parent)
{
    ASSERT(!_parent);
    ASSERT(parent);
    _parent = parent;
}

void Component::SetScheduled(bool schedule)
{
    ASSERT(_parent);
    utl::UpdateQueue::Time time = schedule ? 0 : utl::UpdateQueue::TimeNever;
    if (Sys *sys = eng::Sys::Get()) {
        sys->_updateQueue.Schedule(this, time, 0);
        if (World *world = _parent->GetWorld()) {
            world->_updateQueue.Schedule(this, time, 1);
        }
    }
}

utl::UpdateQueue::Time Component::Update(utl::UpdateQueue::Time time, uintptr_t userData)
{
    return utl::UpdateQueue::TimeNever;
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

void Object::SetScheduled(bool schedule)
{
    for (auto &cmp : _components) {
        cmp->SetScheduled(schedule);
    }
}

void Object::SetWorld(World *world)
{
    ASSERT(bool(world) == !_world);

    if (_world) {
        _world->Update(this, GetBox(), utl::BoxF());
        SetScheduled((bool)world);
    }

    _world = world;

    if (_world) {
        _world->Update(this, utl::BoxF(), GetBox());
        SetScheduled((bool)world);
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