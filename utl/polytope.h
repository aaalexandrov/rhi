#pragma once

#include "geom.h"

namespace utl {

template <typename VecType>
struct Polytope {
	using Vec = VecType;
	using Num = typename VecTraits<Vec>::ElemType;
	static constexpr int Dim = VecTraits<Vec>::Length;
	static constexpr bool IsRound = false;
	using PlaneV = Plane<Vec>;
	using LineV = Line<Vec>;
	using Interval = Box<Num>;
	using BoxV = Box<Vec>;
	using OBoxV = OrientedBox<Vec>;
	using SphereV = Sphere<Vec>;

	struct Edge {
		LineV _line;
		Interval _interval = Interval::GetMaximum();
		std::array<int, 2> _sideIndices;

		template <typename Shape>
		bool Intersects(Shape const &shape) const
		{
			Interval shapeInt = _line.GetIntersection(shape);
			return shapeInt.Intersects(_interval);
		}

		LineV LineFromEndPoints() const { return LineV::FromPoints(_line.GetPoint(_interval._min), _line.GetPoint(_interval._max)); }
	};

	std::vector<PlaneV> _sides;
	std::vector<Edge> _edges;
	std::vector<Vec> _points;
	std::vector<Vec> _sideDirections;
	std::vector<Vec> _edgeDirections;

	void Init(const std::span<PlaneV> sides, Num pointEps = NumericTraits<Num>::Eps, Num vecEps = NumericTraits<Num>::Eps)
	{
		for (PlaneV &plane : sides) {
			AddSide(plane);
		}
		UpdateAfterAddingSides(pointEps, vecEps);
	}

	void AddSide(PlaneV const &side)
	{
		std::vector<Edge> newEdges;
		for (int s = 0; s < _sides.size(); ++s) {
			Edge edge;
			edge._line = _sides[s].GetIntersectionLine(side).Normalized();
			if (!edge._line.IsValid())
				continue;
			edge._sideIndices = { s, (int)_sides.size() };
			newEdges.push_back(edge);
		}

		IntersectEdges(_edges, side, (int)_sides.size());

		for (int s = 0; s < _sides.size(); ++s) {
			IntersectEdges(newEdges, _sides[s], s);
		}

		_edges.insert(_edges.end(), newEdges.begin(), newEdges.end());
		_sides.push_back(side);
	}

	static void IntersectEdges(std::vector<Edge> &edges, PlaneV const &side, int sideIndex)
	{
		for (int e = edges.size() - 1; e >= 0; --e) {
			if (edges[e]._sideIndices[0] == sideIndex || edges[e]._sideIndices[1] == sideIndex)
				continue;
			Interval intersect = edges[e]._line.GetIntersection(side);
			edges[e]._interval = edges[e]._interval.GetIntersection(intersect);
			if (edges[e]._interval.IsEmpty())
				RemoveElement(edges, e);
		}
	}

	void UpdateAfterAddingSides(Num pointEps = NumericTraits<Num>::Eps, Num vecEps = NumericTraits<Num>::Eps)
	{
		_points.clear();
		_sideDirections.clear();
		_edgeDirections.clear();

		for (auto &side : _sides) {
			AddUniqueVec(_sideDirections, glm::normalize(side._normal), true, vecEps);
		}

		for (auto &edge : _edges) {
			Vec p0 = edge._line.GetPoint(edge._interval._min);
			Vec p1 = edge._line.GetPoint(edge._interval._max);
			AddUniqueVec(_points, p0, false, pointEps);
			AddUniqueVec(_points, p1, false, pointEps);
			AddUniqueVec(_edgeDirections, edge._line._direction, true, vecEps);
		}
	}

	static bool AddUniqueVec(std::vector<Vec> &vecs, Vec const &v, bool oppositeIsEqual, Num eps = NumericTraits<Num>::Eps)
	{
		auto found = std::find_if(vecs.begin(), vecs.end(), [&](Vec const &e) { return IsEqual(e, v, eps) || oppositeIsEqual && IsEqual(-e, v, eps); });
		if (found != vecs.end())
			return false;
		vecs.push_back(v);
		return true;
	}

	constexpr int GetNumPoints() const { return static_cast<int>(_points.size()); }
	constexpr Vec GetPoint(int pointInd) const { return _points[pointInd]; }
	constexpr int GetNumSideDirections() const { return static_cast<int>(_sideDirections.size()); }
	constexpr Vec GetSideDirection(int dirInd) const { return _sideDirections[dirInd]; }
	constexpr int GetNumEdgeDirections() const { return static_cast<int>(_edgeDirections.size()); }
	constexpr Vec GetEdgeDirection(int dirInd) const { return _edgeDirections[dirInd]; }
	constexpr int GetNumEdges() const { return static_cast<int>(_edges.size()); }
	constexpr LineV GetEdge(int edgeInd) const { return _edges[edgeInd].LineFromEndPoints(); }

	bool Contains(Vec const &p) const
	{
		for (PlaneV const &side : _sides) {
			// negative side is considered "inside"
			if (side.Eval(p) > 0)
				return false;
		}
		return true;
	}

	template <typename Shape>
	bool Intersects(Shape const &shape) const
	{
		return SatTest<Shape, Polytope>::Intersects(shape, *this);
	}
};

template <typename VecType>
struct PlaneEval<Polytope<VecType>> {
	using Shape = Polytope<VecType>;
	using Vec = typename Shape::Vec;
	using Num = typename VecTraits<Vec>::ElemType;
	using PlaneV = Plane<Vec>;
	using Interval = Box<Num>;

	static constexpr Interval Eval(PlaneV const &plane, Shape const &polytope)
	{
		return plane.Eval(polytope._points);
	}
};

using Polytope3F = Polytope<glm::vec3>;

}