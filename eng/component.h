#pragma once

#include "object.h"
#include "rendering.h"
#include "utl/polytope.h"


namespace eng {

struct CameraCmp : public Component {

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<CameraCmp>(); }

	glm::mat3x4 GetViewMatrix() const;
	glm::mat4   GetProjMatrix(glm::vec2 viewportSize) const;
	utl::Polytope3F GetFrustum() const;

	float _fov = glm::pi<float>();
	float _near = 0.1f, _far = 1000.0f;
};

struct RenderingCmp : public Component {
	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<RenderingCmp>(); }

	std::vector<Model> _models;
};

}