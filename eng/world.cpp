#include "world.h"

namespace eng {

static auto s_regTypes = TypeInfo::AddInitializer("world", [] {
    TypeInfo::Register<World>().Name("eng::World")
        .Base<utl::Any>();
});


void World::Update(Object *obj, utl::BoxF const &curBox, utl::BoxF const &newBox)
{
}

}