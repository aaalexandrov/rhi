#pragma once

#include "geom.h"
#include "polytope.h"

namespace utl {

template <typename VecType>
struct GeomPrimitive {
	using Vec = VecType;
	using Num = typename VecTraits<Vec>::ElemType;
	static constexpr int Dim = VecTraits<Vec>::Length;
	using RotationV = Rotation<Vec>;
	using Rot = typename RotationV::Rot;
	using BoxV = Box<Vec>;
	using OBoxV = OrientedBox<Vec>;
	using SphereV = Sphere<Vec>;
	using PolytopeV = Polytope<Vec>;
	using Transform = RigidTransform<Vec>;

	enum class Kind {
		None,
		Point,
		Box,
		OrientedBox,
		Sphere,
	};

	GeomPrimitive() { Set(); }
	GeomPrimitive(GeomPrimitive const &other) { Set(other); }
	explicit GeomPrimitive(Vec const &p) { Set(p); }
	explicit GeomPrimitive(BoxV const &box) { Set(box); }
	explicit GeomPrimitive(OBoxV const &obox) { Set(obox); }
	explicit GeomPrimitive(SphereV const &sphere) { Set(sphere); }

	GeomPrimitive &operator=(GeomPrimitive const &other) { Set(other); return *this; }

	Kind GetKind() const { return _kind; }

	void Set()
	{
		_kind = Kind::None;
	}

	void Set(Vec const &p)
	{
		_kind = Kind::Point;
		_center = p;
	}

	void Set(BoxV const &box)
	{
		_kind = Kind::Box;
		_center = box.GetCenter();
		_halfExtent = box.GetSize() / (Num)2;
	}

	void Set(OBoxV const &obox)
	{
		_kind = Kind::OrientedBox;
		_center = obox._center;
		_halfExtent = obox._halfSize;
		_orientation = obox._orientation;
	}

	void Set(SphereV const &sphere)
	{
		_kind = Kind::Sphere;
		_center = sphere._center;
		_radius = sphere._radius;
	}

	void Set(GeomPrimitive const &other)
	{
		_kind = other._kind;
		_center = other._center;
		_halfExtent = other._halfExtent;
		_orientation = other._orientation;
		_radius = other._radius;
	}

	Vec GetPoint() const
	{
		ASSERT(_kind == Kind::Point);
		return _center;
	}

	BoxV GetBox() const
	{
		ASSERT(_kind == Kind::Box);
		return BoxV::FromCenterHalfSize(_center, _halfExtent);
	}

	OBoxV GetOrientedBox() const
	{
		ASSERT(_kind == Kind::OrientedBox);
		return OBoxV(_center, _halfExtent, _orientation);
	}

	SphereV GetSphere() const
	{
		ASSERT(_kind == Kind::Sphere);
		return SphereV(_center, _radius);
	}

	BoxV GetBoundingBox() const
	{
		switch (_kind) {
			case Kind::Point:
				return BoxV::FromPoint(_center);
			case Kind::Box:
				return GetBox();
			case Kind::OrientedBox:
				return GetOrientedBox().GetBox();
			case Kind::Sphere:
				return GetSphere().GetBox();
			default:
				ASSERT(0);
				return BoxV();
		}
	}

	GeomPrimitive GetTransformed(Transform const &transform) const
	{
		GeomPrimitive prim;
		switch (_kind) {
			case Kind::Point:
				prim.Set(transform.TransformPoint(GetPoint()));
				break;
			case Kind::Box:
				prim.Set(transform.Transform(GetBox()));
				break;
			case Kind::OrientedBox:
				prim.Set(transform.Transform(GetOrientedBox()));
				break;
			case Kind::Sphere:
				prim.Set(transform.Transform(GetSphere()));
				break;
			default:
				ASSERT(0);
				break;
		}
		return prim;
	}

	bool Intersects(Vec const &p) const
	{
		switch (_kind) {
			case Kind::Point:
				return _center == p;
			case Kind::Box:
				return GetBox().Contains(p);
			case Kind::OrientedBox:
				return GetOrientedBox().Contains(p);
			case Kind::Sphere:
				return GetSphere().Contains(p);
			default:
				ASSERT(0);
				return false;
		}
	}

	bool Intersects(BoxV const &b) const
	{
		switch (_kind) {
			case Kind::Point:
				return b.Contains(_center);
			case Kind::Box:
				return GetBox().Intersects(b);
			case Kind::OrientedBox:
				return GetOrientedBox().Intersects(b);
			case Kind::Sphere:
				return GetSphere().Intersects(b);
			default:
				ASSERT(0);
				return false;
		}
	}

	bool Intersects(OBoxV const &o) const
	{
		switch (_kind) {
			case Kind::Point:
				return o.Contains(_center);
			case Kind::Box:
				return o.Intersects(GetBox());
			case Kind::OrientedBox:
				return GetOrientedBox().Intersects(o);
			case Kind::Sphere:
				return GetSphere().Intersects(o);
			default:
				ASSERT(0);
				return false;
		}
	}

	bool Intersects(SphereV const &s) const
	{
		switch (_kind) {
			case Kind::Point:
				return s.Contains(_center);
			case Kind::Box:
				return s.Intersects(GetBox());
			case Kind::OrientedBox:
				return s.Intersects(GetOrientedBox());
			case Kind::Sphere:
				return GetSphere().Intersects(s);
			default:
				ASSERT(0);
				return false;
		}
	}

	bool Intersects(GeomPrimitive const &prim) const
	{
		switch (_kind) {
			case Kind::Point:
				return prim.Intersects(_center);
			case Kind::Box:
				return prim.Intersects(GetBox());
			case Kind::OrientedBox:
				return prim.Intersects(GetOrientedBox());
			case Kind::Sphere:
				return prim.Intersects(GetSphere());
			default:
				ASSERT(0);
				return false;
		}
	}

	bool Intersects(PolytopeV const &poly) const
	{
		switch (_kind) {
			case Kind::Point:
				return poly.Contains(_center);
			case Kind::Box:
				return poly.Intersects(GetBox());
			case Kind::OrientedBox:
				return poly.Intersects(GetOrientedBox());
			case Kind::Sphere:
				return poly.Intersects(GetSphere());
			default:
				ASSERT(0);
				return false;
		}

	}

private:
	Vec _center, _halfExtent;
	Rot _orientation;
	Num _radius;
	Kind _kind;
};

using GeomPrimitive3F = GeomPrimitive<glm::vec3>;

}