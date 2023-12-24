#pragma once

#include "base.h"

namespace rhi {

struct Resource;
struct Buffer;
struct Texture;
struct Submission;
struct Pipeline;
struct ResourceSet;

struct Pass : public RhiOwned {

	virtual void EnumResources(ResourceEnum enumFn) = 0;

	virtual bool Prepare(Submission *sub) = 0;
	virtual bool Execute(Submission *sub) = 0;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<Pass>(); }
};

struct GraphicsPass : public Pass {
	struct TargetData {
		std::shared_ptr<Texture> _texture;
		glm::vec4 _clearValue{ -1 };
	};

	struct BufferStream {
		std::shared_ptr<Buffer> _buffer;
		size_t _offset = 0;
	};

	struct DrawData {
		std::shared_ptr<Pipeline> _pipeline;
		std::span<std::shared_ptr<ResourceSet>> _resourceSets;
		std::span<BufferStream> _vertexStreams;
		BufferStream _indexStream;
		utl::IntervalU _indices{ 0, 2 };
		utl::IntervalU _instances{ 0, 0 };
		uint32_t _vertexOffset = 0;
	};

	virtual bool Init(std::span<TargetData> rts);

	virtual bool Draw(DrawData const &draw);

	void EnumResources(ResourceEnum enumFn) override;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<GraphicsPass>(); }

	std::vector<TargetData> _renderTargets;
	std::unordered_set<std::shared_ptr<Pipeline>> _pipelines;
	std::unordered_set<std::shared_ptr<ResourceSet>> _resourceSets;

};

struct ComputePass : public Pass {
	virtual bool Init(Pipeline *pipeline, std::span<std::shared_ptr<ResourceSet>> resourceSets, glm::ivec3 numGroups);

	void EnumResources(ResourceEnum enumFn) override;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<ComputePass>(); }

	std::shared_ptr<Pipeline> _pipeline;
	std::vector<std::shared_ptr<ResourceSet>> _resourceSets;
	glm::ivec3 _numGroups{ 0 };
};

struct CopyPass : public Pass {
	union CopyType {
		struct {
			uint8_t srcTex : 1;
			uint8_t dstTex : 1;
		};
		uint8_t _flags = 0;
	};
	struct CopyData {
		ResourceRef _src, _dst;
		CopyType GetCopyType() const;
	};

	virtual bool Copy(CopyData copy);

	virtual bool NeedsMatchingTextures(CopyData &copy) = 0;

	void EnumResources(ResourceEnum enumFn) override;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<CopyPass>(); }

	std::vector<CopyData> _copies;
};

struct Swapchain;
struct PresentPass : public Pass {

	void SetSwapchainTexture(std::shared_ptr<Texture> tex);
	std::shared_ptr<Swapchain> GetSwapchain();

	void EnumResources(ResourceEnum enumFn) override;

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<PresentPass>(); }
	std::shared_ptr<Texture> _swapchainTexture;
	std::shared_ptr<Swapchain> _swapchain;
};

}