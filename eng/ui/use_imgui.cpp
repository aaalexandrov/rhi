#include "use_imgui.h"

#include "rhi/vk/rhi_vk.h"
#include "rhi/vk/graphics_pass_vk.h"

#include "backends/imgui_impl_vulkan.h"

namespace eng {

static rhi::RhiVk *s_rhi = nullptr;
static vk::DescriptorPool s_imguiDescriptorPool;

bool ImguiInit(SDL_Window *window, rhi::GraphicsPass *pass)
{
	auto passVk = static_cast<rhi::GraphicsPassVk *>(pass);
    s_rhi = passVk->_recorder._rhi;

    {
        std::array<vk::DescriptorPoolSize, 1> pool_sizes{ {
            { vk::DescriptorType::eCombinedImageSampler, 10 },
        } };
        vk::DescriptorPoolCreateInfo pool_info{
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            10,
            pool_sizes
        };
        if (s_rhi->_device.createDescriptorPool(&pool_info, s_rhi->AllocCallbacks(), &s_imguiDescriptorPool) != vk::Result::eSuccess)
            return false;
    }

        // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForVulkan(window);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = s_rhi->_instance;
    init_info.PhysicalDevice = s_rhi->_physDevice;
    init_info.Device = s_rhi->_device;
    init_info.QueueFamily = s_rhi->_universalQueue._family;
    init_info.Queue = s_rhi->_universalQueue._queue;
    init_info.PipelineCache = s_rhi->_pipelineCache;
    init_info.DescriptorPool = s_imguiDescriptorPool;
    init_info.Subpass = 0;
    init_info.MinImageCount = 2;
    init_info.ImageCount = 2;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = (VkAllocationCallbacks*)s_rhi->AllocCallbacks();
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


    return true;
}

void ImguiNewFrame()
{
    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

bool ImguiRender(rhi::GraphicsPass *pass)
{
	auto passVk = static_cast<rhi::GraphicsPassVk *>(pass);

    ImGui::Render();
    ImDrawData *draw_data = ImGui::GetDrawData();

    // Record dear imgui primitives into command buffer
    ImGui_ImplVulkan_RenderDrawData(draw_data, passVk->_recorder._cmdBuffers.back());

    return true;
}

void ImguiDone()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    s_rhi->_device.destroyDescriptorPool(s_imguiDescriptorPool, s_rhi->AllocCallbacks());
}


}