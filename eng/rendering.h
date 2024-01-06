#pragma once

#include "rhi/resource.h"
#include "rhi/pipeline.h"
#include "utl/geom_primitive.h"

namespace eng {

using utl::TypeInfo;

struct Mesh : std::enable_shared_from_this<Mesh>, utl::Any {
	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Mesh>(); }

	std::shared_ptr<rhi::Buffer> _vertices;
	std::shared_ptr<rhi::Buffer> _indices;
	std::shared_ptr<TypeInfo> _vertexLayout;
	utl::IntervalU _indexRange;
	rhi::PrimitiveKind _primitiveKind = rhi::PrimitiveKind::TriangleList;
	utl::GeomPrimitive3F _bound;
};

struct Material : std::enable_shared_from_this<Material>, utl::Any {
	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Material>(); }

	std::shared_ptr<rhi::Pipeline> _pipeline;
	std::vector<uint8_t> _params;
	std::shared_ptr<rhi::Buffer> _paramsBuf;
};

struct Model : std::enable_shared_from_this<Model>, utl::Any {
	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Model>(); }

	std::shared_ptr<Mesh> _mesh;
	std::shared_ptr<Material> _material;
};

}