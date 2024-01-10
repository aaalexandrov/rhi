#pragma once

struct ImGuiContext;
union SDL_Event;

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

	void ProcessEvent(Window *window, SDL_Event const &event);

	using InputHandlerFn = std::function<void(Window *window, SDL_Event const &sdlEvent)>;

	ImGuiContext *_ctx = nullptr;
	ImguiRhiData *_rhiData;
	InputHandlerFn _fnInput;
};

}