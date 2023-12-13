#pragma once

#include "base.h"

namespace rhi {

struct Resource;
struct Texture;
struct Submission;

struct Pass : public RhiOwned {

	using ResourceEnum = std::function<void(Resource *, ResourceUsage)>;
	virtual void EnumResources(ResourceEnum enumFn) = 0;

	virtual bool Prepare(Submission *sub) = 0;
	virtual bool Execute(Submission *sub) = 0;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Pass>(); }
};

struct GraphicsPass : public Pass {
	struct TargetData {
		std::shared_ptr<Texture> _texture;
		glm::vec4 _clearValue{ 0 };
	};

	void AddRenderTargets(std::span<TargetData> rts);

	void EnumResources(ResourceEnum enumFn) override;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<GraphicsPass>(); }

	std::vector<TargetData> _renderTargets;
};

struct ComputePass : public Pass {
	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<ComputePass>(); }
};

struct CopyPass : public Pass {
	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<CopyPass>(); }
};

struct PresentPass : public Pass {

	void SetSwapchainTexture(std::shared_ptr<Texture> tex);

	void EnumResources(ResourceEnum enumFn) override;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<PresentPass>(); }
	std::shared_ptr<Texture> _swapchainTexture;
};

}