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


utl::GeomPrimitive3F Mesh::GetVertexBound(std::string positionAttrName, float eps)
{
    size_t streamStride = 0;
    auto verts = _vertices->Map();
    utl::AnyRef posRef{ nullptr, verts.data() };
    for (auto &vertInp : _vertexInputs) {
        auto posMember = vertInp._layout->GetMemberData(positionAttrName);
        if (posMember._type) {
            streamStride = vertInp._layout->_size;
            posRef._type = posMember._type;
            posRef += posMember._offset;
            break;
        }
        posRef += vertInp._layout->_size * _numVertices;
    }
    _vertices->Unmap();
    
    utl::BoxF box;
    utl::SphereF sphere;
    for (int i = 0; i < _numVertices; ++i) {
        glm::vec3 *p = posRef.Get<glm::vec3>();
        box = box.GetUnion(*p);
        sphere = sphere.GetUnion(*p);
    }

    if (box.IsEmpty())
        return utl::GeomPrimitive3F();

    box = box.GetExtended(eps);
    sphere._radius += eps;

    if (box.GetVolume() < sphere.GetVolume())
        return utl::GeomPrimitive3F(box);

    return utl::GeomPrimitive3F(sphere);
}

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