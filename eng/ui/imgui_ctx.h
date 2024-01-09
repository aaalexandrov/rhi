#pragma once

struct ImGuiContext;

namespace rhi {
struct GraphicsPass;
}

namespace eng {

using utl::TypeInfo;

struct Window;

struct ImguiRhiData;

struct ImguiCtx : utl::Any {
	~ImguiCtx();

	TypeInfo const *GetTypeInfo() const override { return TypeInfo::Get<ImguiCtx>(); }

	bool Init(Window *window);

	using ImguiLayoutFn = std::function<void()>;
	void LayoutUi(ImguiLayoutFn fnLayout);

	void Render(rhi::GraphicsPass *graphicsPass);

	ImGuiContext *_ctx = nullptr;
	std::unique_ptr<ImguiRhiData> _rhiData;
};

}