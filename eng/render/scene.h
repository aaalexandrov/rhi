#pragma once

namespace rhi {
struct RenderTargetData;
struct Pass;
struct GraphicsPass;
struct CopyPass;
struct Submission;
struct Pipeline;
struct ResourceSet;
}

namespace eng {

struct World;
struct Object;
struct CameraCmp;
struct RenderingCmp;
struct Scene;

struct RenderObjectsData {
	Scene *_scene = nullptr;
	std::shared_ptr<rhi::GraphicsPass> _renderPass;
	std::vector<std::shared_ptr<rhi::Pass>> _updatePasses;

	glm::ivec2 GetRenderTargetSize() const;
};

struct Scene {
	Scene(World *world, CameraCmp *camera, std::span<rhi::RenderTargetData> renderTargets);

	RenderObjectsData RenderObjects();

	bool RenderObject(Object *obj, RenderObjectsData &renderData);
	bool UpdateSceneParams(RenderObjectsData &renderData);

	using UpdateBufferFn = std::function<bool(utl::AnyRef bufferContent)>;
	static std::shared_ptr<rhi::CopyPass> UpdateResourceSetBuffer(rhi::ResourceSet *resSet, std::string bufName, UpdateBufferFn updateBufferFn);
	static std::shared_ptr<rhi::ResourceSet> CreateResourseSetWithBuffer(rhi::Pipeline *pipeline, uint32_t setIndex, std::string bufName);

	World *_world = nullptr;
	CameraCmp *_camera = nullptr;
	std::vector<rhi::RenderTargetData> _renderTargets;
	std::shared_ptr<rhi::ResourceSet> _sceneParams;
};



}