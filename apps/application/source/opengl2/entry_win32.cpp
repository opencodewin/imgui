#if defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include <windows.h>
#include <tchar.h>
#include <vector>
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_opengl2.h"
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <string>
#include <cerrno>
#include "application.h"
#if IMGUI_VULKAN_SHADER
#include <ImVulkanShader.h>
#endif

static void * user_handle = nullptr;

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0 // From Windows SDK 8.1+ headers
#endif

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED)
            {
            }
            return 0;
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
                return 0;
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_DPICHANGED:
            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports)
            {
                //const int dpi = HIWORD(wParam);
                //printf("WM_DPICHANGED to %d (%.0f%%)\n", dpi, (float)dpi / 96.0f * 100.0f);
                const RECT* suggested_rect = (RECT*)lParam;
                ::SetWindowPos(hWnd, NULL, suggested_rect->left, suggested_rect->top, suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
            }
            break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

# if defined(_UNICODE)
std::wstring widen(const std::string& str)
{
    int size = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), nullptr, 0);
    std::wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), (wchar_t*)result.data(), size);
    return result;
}
# endif

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    PIXELFORMATDESCRIPTOR pfd;
    ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));
    const auto c_ClassName  = _T("Imgui Application Class");
# if defined(_UNICODE)
    const std::wstring c_WindowName = widen(Application_GetName(user_handle));
# else
    const std::string c_WindowName = Application_GetName(user_handle) + std::string(" Win32_GL2");
# endif

# if defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
# endif
    // Create application window
    const auto wc = WNDCLASSEX{ sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), LoadIcon(GetModuleHandle(nullptr), IDI_APPLICATION),
        LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr, c_ClassName, LoadIcon(GetModuleHandle(nullptr), IDI_APPLICATION) };
    RegisterClassEx(&wc);

    int window_width = 1440;
    int window_height = 960;
    float window_scale = 1;

    auto hwnd = CreateWindow(c_ClassName, c_WindowName.c_str(), WS_OVERLAPPEDWINDOW, 100, 100, window_width, window_height, nullptr, nullptr, wc.hInstance, nullptr);
    if (hwnd == nullptr)
    {
        fprintf(stderr, "Failed to Open window! %s\n", c_WindowName.c_str());
        return 1;
    }
    auto dc = GetDC(hwnd);
    if (dc == nullptr)
    {
        fprintf(stderr, "Failed to get window DC! %s\n", c_WindowName.c_str());
        return 1;
    }
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    auto visual = ChoosePixelFormat(dc, &pfd);
    if (0 == visual)
    {
        fprintf(stderr, "Failed to Choose PixelFormat! %s\n", c_WindowName.c_str());
        return 1;
    }
    if (FALSE == SetPixelFormat(dc, visual, &pfd))
    {
        fprintf(stderr, "Failed to Set PixelFormat! %s\n", c_WindowName.c_str());
        return 1;
    }
    auto rc = wglCreateContext(dc);
    if (rc == nullptr)
    {
        fprintf(stderr, "Failed to create context! %s\n", c_WindowName.c_str());
        return 1;
    }
    if (FALSE == wglMakeCurrent(dc, rc))
    {
        fprintf(stderr, "Failed to Make Current context! %s\n", c_WindowName.c_str());
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.FontGlobalScale = window_scale;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImVec4 clear_color = ImVec4(0.f, 0.f, 0.f, 1.f);

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplOpenGL2_Init();

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    Application_Initialize(&user_handle);

    // Main loop
    bool done = false;
    while (!done)
    {
        ImGui_ImplWin32_WaitForEvent();
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (io.ConfigFlags & ImGuiConfigFlags_EnableLowRefreshMode)
            ImGui::SetMaxWaitBeforeNextFrame(1.0 / 30.0);

        done = Application_Frame(user_handle);
        if (done)
            ::PostQuitMessage(0);

        ImGui::EndFrame();

        // Rendering
        ImGui::Render();
        ImGui_ImplOpenGL2_ClearScreen(ImVec2(0, 0), io.DisplaySize, clear_color);
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        SwapBuffers(dc);
    }

    Application_Finalize(&user_handle);

    // Cleanup
#if IMGUI_VULKAN_SHADER
    ImGui::ImVulkanShaderClear();
#endif
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(wglGetCurrentContext());
    ReleaseDC(hwnd, dc);
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}
