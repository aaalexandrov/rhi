#pragma once

namespace rhi {
struct Pass;
struct GraphicsPass;
struct Submission;
struct ResourceSet;
}

namespace eng {

struct World;
struct Object;
struct CameraCmp;
struct RenderingCmp;

struct RenderObjectsData {
	std::shared_ptr<rhi::GraphicsPass> _renderPass;
	std::vector<std::shared_ptr<rhi::Pass>> _updatePasses;
};

struct Renderer {

	bool RenderObjects(World *world, CameraCmp const *camera, RenderObjectsData &renderData);

	bool RenderObject(Object *obj, RenderObjectsData &renderData);
};

}