#pragma once

#include "object.h"
#include "render/rendering.h"
#include "utl/polytope.h"


namespace eng {

struct CameraCmp : public Component {

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<CameraCmp>(); }

	glm::mat4x3 GetViewMatrix() const;
	glm::mat4   GetProjMatrix(glm::vec2 viewportSize) const;
	utl::Polytope3F GetFrustum(glm::vec2 viewportSize) const;

	float _fovY = glm::pi<float>() / 3;
	float _near = 0.1f, _far = 1000.0f;
};

struct RenderObjectsData;
struct RenderingCmp : public Component {

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<RenderingCmp>(); }

	std::vector<Model> _models;
	std::shared_ptr<rhi::ResourceSet> _objParams;

	bool UpdateObjParams(RenderObjectsData &renderData);
};

}