#include "imgui_ctx.h"
#include "eng/sys.h"

#include "rhi/vk/rhi_vk.h"
#include "rhi/vk/graphics_pass_vk.h"

#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"
#include "backends/imgui_impl_sdl2.h"

#include "SDL2/SDL_events.h"
#include "SDL2/SDL_version.h"

namespace eng {

static auto s_regTypes = TypeInfo::AddInitializer("imgui_ctx", [] {
    TypeInfo::Register<ImguiCtx>().Name("eng::ImguiCtx")
        .Base<utl::Any>();
});


struct ImguiRhiData {
    vk::DescriptorPool _descriptorPool;

    ~ImguiRhiData() {
        auto *rhiVk = Cast<rhi::RhiVk>(Sys::Get()->_rhi.get());
        if (_descriptorPool)
            rhiVk->_device.destroyDescriptorPool(_descriptorPool, rhiVk->AllocCallbacks());
    }

    bool Init() {
        auto *rhiVk = Cast<rhi::RhiVk>(Sys::Get()->_rhi.get());
        std::array<vk::DescriptorPoolSize, 1> pool_sizes{ {
            { vk::DescriptorType::eCombinedImageSampler, 10 },
        } };
        vk::DescriptorPoolCreateInfo pool_info{
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            10,
            pool_sizes
        };
        if (rhiVk->_device.createDescriptorPool(&pool_info, rhiVk->AllocCallbacks(), &_descriptorPool) != vk::Result::eSuccess)
            return false;

        return true;
    }
};

ImguiCtx::~ImguiCtx()
{
    if (_ctx) {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplSDL2_Shutdown();

        ImGui::DestroyContext(_ctx);
    }
    delete _rhiData;
    _rhiData = nullptr;
}

bool ImguiCtx::Init(Window *window)
{
    ASSERT(!_rhiData);
    _rhiData = new ImguiRhiData();
    if (!_rhiData->Init()) {
        delete _rhiData;
        _rhiData = nullptr;
        return false;
    }

    ASSERT(!_ctx);
    _ctx = ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    auto *rhiVk = Cast<rhi::RhiVk>(Sys::Get()->_rhi.get());
    // dummy render pass to initialize the imgui internal pipeline
    std::array<rhi::GraphicsPass::TargetData, 1> passRts{ {	window->_swapchain->_images[0] } };
    auto passVk = rhiVk->New<rhi::GraphicsPassVk>("solidPass", std::span(passRts));


    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForVulkan(window->_window);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = rhiVk->_instance;
    init_info.PhysicalDevice = rhiVk->_physDevice;
    init_info.Device = rhiVk->_device;
    init_info.QueueFamily = rhiVk->_universalQueue._family;
    init_info.Queue = rhiVk->_universalQueue._queue;
    init_info.PipelineCache = rhiVk->_pipelineCache;
    init_info.DescriptorPool = _rhiData->_descriptorPool;
    init_info.Subpass = 0;
    init_info.MinImageCount = 2;
    init_info.ImageCount = 2;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = (VkAllocationCallbacks *)rhiVk->AllocCallbacks();
    init_info.CheckVkResultFn = [](VkResult err) { ASSERT(err == VK_SUCCESS); };
    ImGui_ImplVulkan_Init(&init_info, passVk->_renderPass);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    ImGui::SetCurrentContext(nullptr);

    return true;
}

void ImguiCtx::LayoutUi(ImguiLayoutFn fnLayout)
{
    ASSERT(_ctx);
    auto *prevCtx = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(_ctx);

    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    fnLayout();

    ImGui::SetCurrentContext(prevCtx);
}

void ImguiCtx::Render(rhi::GraphicsPass *graphicsPass)
{
    auto *passVk = Cast<rhi::GraphicsPassVk>(graphicsPass);

    auto *prevCtx = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(_ctx);

    ImGui::Render();
    ImDrawData *draw_data = ImGui::GetDrawData();

    // Record dear imgui primitives into command buffer
    ImGui_ImplVulkan_RenderDrawData(draw_data, passVk->_recorder._cmdBuffers.back());

}

void ImguiCtx::ProcessEvent(Window *window, SDL_Event const &event)
{
    auto *prevCtx = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(_ctx);

    ImGui_ImplSDL2_ProcessEvent(&event);

    ImGuiIO &io = ImGui::GetIO();
    bool shouldDispatch = true;
    switch (event.type) {
        case SDL_MOUSEMOTION:
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEWHEEL:
            shouldDispatch = !io.WantCaptureMouse;
            break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
        case SDL_KEYMAPCHANGED:
        case SDL_TEXTEDITING:
#if SDL_VERSION_ATLEAST(2, 0, 22)
        case SDL_TEXTEDITING_EXT:
#endif        
        case SDL_TEXTINPUT:
            shouldDispatch = !io.WantCaptureKeyboard;
            break;
    }

    if (shouldDispatch && _fnInput) {
        _fnInput(window, event);
    }

    ImGui::SetCurrentContext(prevCtx);
}

}