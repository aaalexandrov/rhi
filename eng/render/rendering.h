#pragma once

#include "rhi/resource.h"
#include "rhi/pipeline.h"
#include "rhi/pass.h"
#include "utl/geom_primitive.h"

namespace eng {

using utl::TypeInfo;

struct Mesh : std::enable_shared_from_this<Mesh>, utl::Any {
	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Mesh>(); }

	utl::GeomPrimitive3F GetVertexBound(std::string positionAttrName, float eps = utl::NumericTraits<float>::Eps);

	bool SetGeometryData(rhi::GraphicsPass::DrawData &drawData, std::vector<rhi::GraphicsPass::BufferStream> &vertexStreams);

	std::string _name;
	std::shared_ptr<rhi::Buffer> _vertices;
	std::vector<rhi::VertexInputData> _vertexInputs;
	uint32_t _numVertices;
	std::shared_ptr<rhi::Buffer> _indices;
	utl::IntervalU _indexRange;
	rhi::PrimitiveKind _primitiveKind = rhi::PrimitiveKind::TriangleList;
	utl::GeomPrimitive3F _bound;
};

struct Material : std::enable_shared_from_this<Material>, utl::Any {
	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Material>(); }

	bool UpdateMaterialParams(rhi::Pipeline *pipeline);

	std::string _name;
	std::vector<std::shared_ptr<rhi::Shader>> _shaders;
	rhi::RenderState _renderState;
	std::unordered_map<std::string, std::shared_ptr<rhi::Bindable>> _params;
	std::shared_ptr<rhi::ResourceSet> _materialParams;
	bool _paramsDirty = true;
};

struct Model : std::enable_shared_from_this<Model>, utl::Any {
	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Model>(); }

	std::shared_ptr<Mesh> _mesh;
	std::shared_ptr<Material> _material;
	std::shared_ptr<rhi::Pipeline> _pipeline;
};

}