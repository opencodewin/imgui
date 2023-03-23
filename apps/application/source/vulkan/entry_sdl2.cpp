#include "imgui.h"
#include "imgui_helper.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"
#include <stdio.h>          // printf, fprintf
#include <stdlib.h>         // abort
#include <string>
#include <memory>
#include <SDL.h>
#include <SDL_vulkan.h>
#include "application.h"
#if IMGUI_VULKAN_SHADER
#include <ImVulkanShader.h>
#endif
#include "entry_vulkan.h"

void Application_FullScreen(bool on)
{
    ImGui_ImplSDL2_FullScreen(ImGui::GetMainViewport(), on);
}

static void Show_Splash_Window(ApplicationWindowProperty& property, ImGuiContext* ctx)
{
    std::string title = property.name + " Splash";
    int window_flags = SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP;
    SDL_Window* window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                                                        property.splash_screen_width, property.splash_screen_height, window_flags);

    if (!window)
    {
        fprintf(stderr, "Failed to Create Splash Window: %s\n", SDL_GetError());
        return;
    }

    // Set window icon
    if (!property.icon_path.empty())
    {
        ImGui_ImplSDL2_SetWindowIcon(window, property.icon_path.c_str());
    }

    // Set window alpha
    SDL_SetWindowOpacity(window, property.splash_screen_alpha);

    // Setup Vulkan
    uint32_t extensions_count = 0;
    SDL_Vulkan_GetInstanceExtensions(window, &extensions_count, NULL);
    const char** ext = new const char*[extensions_count];
    SDL_Vulkan_GetInstanceExtensions(window, &extensions_count, ext);
    std::vector<const char*> extensions;
    for (int i = 0; i < extensions_count; i++)
        extensions.push_back(ext[i]);
    SetupVulkan(extensions);
    delete[] ext;

    // Create Window Surface
    VkSurfaceKHR surface;
    VkResult err;
    if (SDL_Vulkan_CreateSurface(window, g_Instance, &surface) == 0)
    {
        printf("Failed to create Vulkan surface.\n");
        return;
    }

    // Create Framebuffers
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
    SetupVulkanWindow(wd, surface, w, h);

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForVulkan(window);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = g_Instance;
    init_info.PhysicalDevice = g_PhysicalDevice;
    init_info.Device = g_Device;
    init_info.QueueFamily = g_QueueFamily;
    init_info.Queue = g_Queue;
    init_info.PipelineCache = g_PipelineCache;
    init_info.DescriptorPool = g_DescriptorPool;
    init_info.Allocator = g_Allocator;
    init_info.MinImageCount = g_MinImageCount;
    init_info.ImageCount = wd->ImageCount;
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info, wd->RenderPass);
    UpdateVulkanFont(wd);

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    if (property.application.Application_SetupContext)
        property.application.Application_SetupContext(ctx, true);
    
    static int32_t frame_count = 0;
    bool done = false;
    bool splash_done = false;
    bool show = true;
    while (!splash_done)
    {
        ImGui_ImplSDL2_WaitForEvent();
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SHOWN)
            {
                show = true;
            }
            if (event.type == SDL_WINDOWEVENT && (event.window.event == SDL_WINDOWEVENT_HIDDEN || event.window.event == SDL_WINDOWEVENT_MINIMIZED))
            {
                show = false;
            }
            if (event.type == SDL_WINDOWEVENT && (event.window.event == SDL_WINDOWEVENT_EXPOSED || event.window.event == SDL_WINDOWEVENT_RESTORED))
            {
                show = true;
            }
        }
        if (!show && !(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
        {
            ImGui::sleep(10);
            continue;
        }

        // Resize swap chain?
        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        if (g_SwapChainRebuild || width > property.width || height > property.height)
        {
            if (width > 0 && height > 0)
            {
                ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
                ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, &g_MainWindowData, g_QueueFamily, g_Allocator, width, height, g_MinImageCount);
                g_MainWindowData.FrameIndex = 0;
                g_SwapChainRebuild = false;
                property.width = width;
                property.height = height;
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        if (io.ConfigFlags & ImGuiConfigFlags_EnableLowRefreshMode)
            ImGui::SetMaxWaitBeforeNextFrame(1.0 / property.fps);
        
        auto _splash_done = property.application.Application_SplashScreen(property.handle, done);
        
        // work around with context assert frame_count
        frame_count ++;
        if (frame_count > 1) splash_done = _splash_done;
        
        ImGui::EndFrame();
        // Rendering
        ImGui::Render();
        FrameRendering(wd);
    }

    err = vkDeviceWaitIdle(g_Device);
    check_vk_result(err);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    CleanupVulkanWindow();
    CleanupVulkan();
    SDL_DestroyWindow(window);
    ImGui::UpdatePlatformWindows();
}

int main(int argc, char** argv)
{
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // Setup window
    ApplicationWindowProperty property(argc, argv);
    Application_Setup(property);

    // Init IME effect windows only
    ImGui_ImplSDL2_InitIme();

    int window_flags = SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI;
    if (property.resizable) window_flags |= SDL_WINDOW_RESIZABLE;
    if (property.full_size)
    {
        SDL_DisplayMode DM;
        SDL_GetCurrentDisplayMode(0, &DM);
        SDL_Rect r;
        SDL_GetDisplayUsableBounds(0, &r);
        property.pos_x = (r.x > 0 && r.x < 100) ? r.x : r.x + FULLSCREEN_OFFSET_X;
        property.pos_y = r.y + FULLSCREEN_OFFSET_Y;
        property.width = DM.w - FULLSCREEN_WIDTH_ADJ;
        property.height = DM.h - r.y;
        property.center = false;
        window_flags |= SDL_WINDOW_BORDERLESS;
    }
    else if (property.full_screen)
    {
        window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }
    else
    {
        if (property.top_most)
        {
            window_flags |= SDL_WINDOW_ALWAYS_ON_TOP;
        }
        if (!property.window_border)
        {
            window_flags |= SDL_WINDOW_BORDERLESS;
        }
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    auto ctx = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGuiContext& g = *GImGui;
    io.Fonts->AddFontDefault(property.font_scale);
    io.FontGlobalScale = 1.0f / property.font_scale;
    if (property.power_save) io.ConfigFlags |= ImGuiConfigFlags_EnableLowRefreshMode;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    // Setup App setting file path
    auto setting_path = property.using_setting_path ? ImGuiHelper::settings_path(property.name) : "";
    auto ini_name = property.name;
    std::replace(ini_name.begin(), ini_name.end(), ' ', '_');
    setting_path += ini_name + ".ini";
    io.IniFilename = setting_path.c_str();
    auto language_path = property.language_path + ini_name + "_language.ini";
    if (property.internationalize)
    {
        io.LanguageFileName = language_path.c_str();
        g.Style.TextInternationalize = 1;
        g.LanguageName = "Default";
    }
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // first call application initialize
    if (property.application.Application_Initialize)
        property.application.Application_Initialize(&property.handle);

    // start splash screen if setting
    bool splash_done = false;
#ifndef __EMSCRIPTEN__
    if (property.application.Application_SplashScreen &&
        property.splash_screen_width > 0 &&
        property.splash_screen_height > 0)
    {
        Show_Splash_Window(property, ctx);
        splash_done = true;
    }
#endif

    std::string title = property.name;
    title += " Vulkan SDL";
    SDL_Window* window = SDL_CreateWindow(title.c_str(), property.center ? SDL_WINDOWPOS_CENTERED : property.pos_x, 
                                                        property.center ? SDL_WINDOWPOS_CENTERED : property.pos_y, 
                                                        property.width, property.height, window_flags);
    if (!window)
    {
        fprintf(stderr, "Failed to Create Window: %s\n", SDL_GetError());
        return -1;
    }

    // Set window icon
    if (!property.icon_path.empty())
    {
        ImGui_ImplSDL2_SetWindowIcon(window, property.icon_path.c_str());
    }

    // Setup Vulkan
    uint32_t extensions_count = 0;
    SDL_Vulkan_GetInstanceExtensions(window, &extensions_count, NULL);
    const char** ext = new const char*[extensions_count];
    SDL_Vulkan_GetInstanceExtensions(window, &extensions_count, ext);
    std::vector<const char*> extensions;
    for (int i = 0; i < extensions_count; i++)
        extensions.push_back(ext[i]);
    SetupVulkan(extensions);
    delete[] ext;

    // Create Window Surface
    VkSurfaceKHR surface;
    VkResult err;
    if (SDL_Vulkan_CreateSurface(window, g_Instance, &surface) == 0)
    {
        printf("Failed to create Vulkan surface.\n");
        return 1;
    }
    
    // Create Framebuffers
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
    SetupVulkanWindow(wd, surface, w, h);

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    // setup imgui docking viewport
    if (property.docking) io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    if (property.viewport) io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    if (!property.auto_merge) io.ConfigViewportsNoAutoMerge = true;
    if (!splash_done && property.application.Application_SetupContext)
        property.application.Application_SetupContext(ctx, false);
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForVulkan(window);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = g_Instance;
    init_info.PhysicalDevice = g_PhysicalDevice;
    init_info.Device = g_Device;
    init_info.QueueFamily = g_QueueFamily;
    init_info.Queue = g_Queue;
    init_info.PipelineCache = g_PipelineCache;
    init_info.DescriptorPool = g_DescriptorPool;
    init_info.Allocator = g_Allocator;
    init_info.MinImageCount = g_MinImageCount;
    init_info.ImageCount = wd->ImageCount;
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info, wd->RenderPass);

    UpdateVulkanFont(wd);

#if IMGUI_VULKAN_SHADER
    ImGui::ImVulkanShaderInit();
#endif

    // Main loop
    bool done = false;
    bool app_done = false;
    bool show = true;
    while (!app_done)
    {
        ImGui_ImplSDL2_WaitForEvent();
        SDL_Event event;
        std::vector<std::string> paths;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SHOWN)
            {
                show = true;
            }
            if (event.type == SDL_WINDOWEVENT && (event.window.event == SDL_WINDOWEVENT_HIDDEN || event.window.event == SDL_WINDOWEVENT_MINIMIZED))
            {
                show = false;
            }
            if (event.type == SDL_WINDOWEVENT && (event.window.event == SDL_WINDOWEVENT_EXPOSED || event.window.event == SDL_WINDOWEVENT_RESTORED))
            {
                show = true;
            }
            if (event.type == SDL_DROPFILE)
            {
                // file path in event.drop.file
                paths.push_back(event.drop.file);
                show = true;
            }
        }
        if (!show && !(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
        {
            ImGui::sleep(10);
            continue;
        }
        if (!paths.empty())
        {
            if (property.application.Application_DropFromSystem)
                property.application.Application_DropFromSystem(paths);
            paths.clear();
        }

        // Resize swap chain?
        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        if (g_SwapChainRebuild || width > property.width || height > property.height)
        {
            if (width > 0 && height > 0)
            {
                ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
                ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, &g_MainWindowData, g_QueueFamily, g_Allocator, width, height, g_MinImageCount);
                g_MainWindowData.FrameIndex = 0;
                g_SwapChainRebuild = false;
                property.width = width;
                property.height = height;
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        if (io.ConfigFlags & ImGuiConfigFlags_EnableLowRefreshMode)
            ImGui::SetMaxWaitBeforeNextFrame(1.0 / property.fps);

        if (property.application.Application_Frame)
            app_done = property.application.Application_Frame(property.handle, done);
        else
            app_done = done;

        ImGui::EndFrame();
        // Rendering
        ImGui::Render();
        FrameRendering(wd);
    }

    if (property.application.Application_Finalize)
        property.application.Application_Finalize(&property.handle);

    // Cleanup
#if IMGUI_VULKAN_SHADER
    //ImGui::ImVulkanShaderClear();
#endif
    err = vkDeviceWaitIdle(g_Device);
    check_vk_result(err);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    CleanupVulkanWindow();
#if IMGUI_VULKAN_SHADER
    ImGui::ImVulkanShaderClear();
    CleanupVulkan(true);
#else
    CleanupVulkan();
#endif
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
