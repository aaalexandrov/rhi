#include "rendering.h"

namespace eng {

static auto s_regTypes = TypeInfo::AddInitializer("rendering", [] {
    TypeInfo::Register<Mesh>().Name("eng::Mesh")
        .Base<utl::Any>();
    TypeInfo::Register<Material>().Name("eng::Material")
        .Base<utl::Any>();
    TypeInfo::Register<Model>().Name("eng::Model")
        .Base<utl::Any>();
});


}