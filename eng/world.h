#pragma once

namespace eng {

using utl::TypeInfo;

struct Object;
struct World : utl::Any {

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<World>(); }

	void Update(Object *obj, utl::BoxF const &curBox, utl::BoxF const &newBox);
};

}