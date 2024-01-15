#include "renderer.h"
#include "eng/world.h"
#include "eng/component.h"
#include "rhi/pass.h"
#include "rhi/resource.h"

namespace eng {

bool Renderer::RenderObjects(World *world, CameraCmp const *camera, RenderObjectsData &renderData)
{
	glm::ivec2 rtSize = renderData._renderPass->_renderTargets[0]._texture->_descriptor._dimensions;
	utl::Polytope3F frustum = camera->GetFrustum(rtSize);
	bool res = true;
	world->EnumObjects(frustum, [&](std::shared_ptr<Object> &obj) {
		res = RenderObject(obj.get(), renderData) && res;
		ASSERT(res);
		return utl::Enum::Continue;
	});

	return res;
}

bool Renderer::RenderObject(Object *obj, RenderObjectsData &renderData)
{
	auto *renderCmp = obj->GetComponent<RenderingCmp>();
	if (!renderCmp)
		return true;

	if (!renderCmp->UpdateObjParams(renderData))
		return false;

	bool res = true;
	std::vector<rhi::GraphicsPass::BufferStream> vertexStreams;
	std::vector<std::shared_ptr<rhi::ResourceSet>> resourceSets;
	for (auto &model : renderCmp->_models) {
		rhi::GraphicsPass::DrawData drawData;
		drawData._pipeline = model._pipeline;

		resourceSets.insert(resourceSets.end(), { renderCmp->_objParams, model._materialParams });
		drawData._resourceSets = resourceSets;

		res = model._mesh->SetGeometryData(drawData, vertexStreams) && res;
		ASSERT(res);

		res = renderData._renderPass->Draw(drawData) && res;
		ASSERT(res);
		vertexStreams.clear();
		resourceSets.clear();
	}

	return res;
}

}