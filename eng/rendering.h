#pragma once

#include "rhi/resource.h"
#include "rhi/pipeline.h"
#include "utl/geom_primitive.h"

namespace eng {

using utl::TypeInfo;

struct Mesh : std::enable_shared_from_this<Mesh>, utl::Any {
	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Mesh>(); }

	std::shared_ptr<rhi::Buffer> _vertices;
	std::vector<rhi::VertexInputData> _vertexInputs;
	std::shared_ptr<rhi::Buffer> _indices;
	utl::IntervalU _indexRange;
	rhi::PrimitiveKind _primitiveKind = rhi::PrimitiveKind::TriangleList;
	utl::GeomPrimitive3F _bound;
};

struct Material : std::enable_shared_from_this<Material>, utl::Any {
	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Material>(); }

	std::vector<std::shared_ptr<rhi::Shader>> _shaders;
	rhi::RenderState _renderState;
	std::unordered_map<std::string, std::shared_ptr<rhi::Bindable>> _params;
};

struct Model : std::enable_shared_from_this<Model>, utl::Any {
	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Model>(); }

	std::shared_ptr<Mesh> _mesh;
	std::shared_ptr<Material> _material;
	std::shared_ptr<rhi::Pipeline> _pipeline;
	std::shared_ptr<rhi::ResourceSet> _materialParams;
};

}