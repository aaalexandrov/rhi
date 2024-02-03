#include "scene.h"
#include "eng/sys.h"
#include "eng/world.h"
#include "eng/component.h"
#include "rhi/pass.h"
#include "rhi/resource.h"

namespace eng {

glm::ivec2 RenderObjectsData::GetRenderTargetSize() const
{
	return _renderPass->_renderTargets[0]._texture->_descriptor._dimensions;
}

Scene::Scene(World *world, CameraCmp *camera, std::span<rhi::RenderTargetData> renderTargets)
	: _world(world)
	, _camera(camera)
	, _renderTargets(renderTargets.begin(), renderTargets.end())
{
}

RenderObjectsData Scene::RenderObjects()
{
	rhi::Rhi *rhi = Sys::Get()->_rhi.get();
	RenderObjectsData renderData{
		._scene = this,
		._renderPass = rhi->New<rhi::GraphicsPass>(_world->_name, std::span(_renderTargets)),
	};

	bool res = UpdateSceneParams(renderData);
	ASSERT(res);

	utl::Polytope3F frustum = _camera->GetFrustum(renderData.GetRenderTargetSize());
	_world->EnumObjects(frustum, [&](std::shared_ptr<Object> &obj) {
		bool res = RenderObject(obj.get(), renderData);
		ASSERT(res);
		return utl::Enum::Continue;
	});

	return renderData;
}

bool Scene::RenderObject(Object *obj, RenderObjectsData &renderData)
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

		if (!model._material->UpdateMaterialParams(model._pipeline.get()))
			return false;

		resourceSets.insert(resourceSets.end(), { renderCmp->_objParams, model._material->_materialParams, renderData._scene->_sceneParams });
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

bool Scene::UpdateSceneParams(RenderObjectsData &renderData)
{
	if (!_sceneParams) {
		_world->EnumObjects([&](std::shared_ptr<Object> &obj) {
			auto *renderCmp = obj->GetComponent<RenderingCmp>();
			if (renderCmp && renderCmp->_models.size()) {
				_sceneParams = CreateResourseSetWithBuffer(renderCmp->_models[0]._pipeline.get(), 2, "SceneData");
				return utl::Enum::Stop;
			}
			return utl::Enum::Continue;
		});
	}

	auto uploadPass = Scene::UpdateResourceSetBuffer(_sceneParams.get(), "SceneData", [&](utl::AnyRef params) {
		*params.GetMember("view").Get<glm::mat4>() = _camera->GetViewMatrix();
		*params.GetMember("proj").Get<glm::mat4>() = _camera->GetProjMatrix(renderData.GetRenderTargetSize());
		return true;
	});
	renderData._updatePasses.push_back(std::move(uploadPass));

	return true;
}

std::shared_ptr<rhi::CopyPass> Scene::UpdateResourceSetBuffer(rhi::ResourceSet *resSet, std::string bufName, UpdateBufferFn updateBufferFn)
{
	std::shared_ptr<rhi::CopyPass> uploadPass;
	int32_t transformsParam = resSet->GetSetDescription()->GetParamIndex(bufName);
	if (transformsParam >= 0) {
		rhi::ResourceSetDescription::Param const &param = resSet->GetSetDescription()->_params[transformsParam];
		ASSERT(param._kind == rhi::ShaderParam::UniformBuffer || param._kind == rhi::ShaderParam::UAVBuffer);
		rhi::ShaderParam const *shaderParam = resSet->_pipeline->GetShaderParam(resSet->_setIndex, transformsParam);

		rhi::Rhi *rhi = resSet->_pipeline->_rhi;

		rhi::ResourceDescriptor bufDesc{
			._usage{.copySrc = 1, .cpuAccess = 1, },
			._dimensions{ (int32_t)shaderParam->_type->_size },
		};
		auto uploadBuf = rhi->New<rhi::Buffer>("Update" + shaderParam->_name, bufDesc);
		auto uploadData = uploadBuf->Map();

		utl::AnyRef transforms{ shaderParam->_type, uploadData.data() };
		bool res = updateBufferFn(transforms);

		uploadBuf->Unmap();

		if (res) {
			uploadPass = rhi->Create<rhi::CopyPass>("Upload" + shaderParam->_name);
			bool res = uploadPass->Copy(rhi::CopyPass::CopyData{ ._src{uploadBuf}, ._dst{resSet->_resourceRefs[transformsParam]._bindable} });
			ASSERT(res);
		}
	}

	return uploadPass;
}

std::shared_ptr<rhi::ResourceSet> Scene::CreateResourseSetWithBuffer(rhi::Pipeline *pipeline, uint32_t setIndex, std::string bufName)
{
	std::shared_ptr<rhi::ResourceSet> resSet = pipeline->AllocResourceSet(setIndex);
	rhi::ShaderParam const *param = pipeline->GetShaderParam(setIndex, bufName);

	rhi::ResourceDescriptor paramBufDesc{
		._usage = rhi::ResourceUsage{.srv = 1, .uav = (param->_kind == rhi::ShaderParam::UAVBuffer), .copyDst = 1},
		._dimensions = glm::ivec4{(int32_t)param->_type->_size, 0, 0, 0},
	};
	auto paramBuf = pipeline->_rhi->New<rhi::Buffer>(param->_name, paramBufDesc);

	resSet->_resourceRefs[param->_binding]._bindable = std::move(paramBuf);
	resSet->Update();

	return resSet;
}

}