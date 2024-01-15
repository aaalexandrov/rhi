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


bool Mesh::SetGeometryData(rhi::GraphicsPass::DrawData &drawData, std::vector<rhi::GraphicsPass::BufferStream> &vertexStreams)
{
    ASSERT(drawData._pipeline);

    ASSERT(vertexStreams.empty());
    ASSERT(_vertexInputs.size() == drawData._pipeline->_pipelineData._vertexInputs.size());
    uint32_t streamOffs = 0;
    for (uint32_t s = 0; s < _vertexInputs.size(); ++s) {
        vertexStreams.push_back({_vertices, streamOffs});
        ASSERT(!_vertexInputs[s]._perInstance);
        streamOffs += _vertexInputs[s]._layout->_size * _numVertices;
    }
    drawData._vertexStreams = vertexStreams;

    drawData._indexStream = { _indices };
    drawData._indices = _indexRange;

    ASSERT(drawData._instances.IsEmpty());
    ASSERT(drawData._vertexOffset == 0);

    return true;
}

}