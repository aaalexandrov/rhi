#pragma once

#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"

namespace rhi {
struct GraphicsPass;
}

namespace eng {

bool ImguiInit(SDL_Window *window, rhi::GraphicsPass *pass);
void ImguiNewFrame();
bool ImguiRender(rhi::GraphicsPass *pass);
void ImguiDone();

}