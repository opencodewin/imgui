// Dear ImGui: standalone example application for SDL2 + OpenGL
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// (GL3W is a helper library to access OpenGL functions since there is no standard header to access modern OpenGL functions easily. Alternatives are GLEW, Glad, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#if IMGUI_ADDON_IMPLOTS
#include "implot.h"
#endif
#if IMGUI_ADDON_MARKDOWN
#include "imgui_markdown.h"
#endif
#if IMGUI_ADDON_MEMORY_EDITOR
#include "imgui_memory_editor.h"
#endif
#if IMGUI_ADDON_IMNODES
#include "imnodes.h"
#endif
#if IMGUI_ADDON_NODE_GRAPH
#include "ImGuiNodeGraphEditor.h"
#endif
#if IMGUI_ADDON_TEXT_EDITOR
#include "TextEditor.h"
#endif
#if IMGUI_ADDON_FILE_DIALOG
#include "ImGuiFileDialog.h"
#endif
#if IMGUI_ADDON_FILE_SYSTEM
#include "ImGuiFileSystem.h"
#endif
#if IMGUI_ADDON_DOCK
#include "imgui_dock.h"
#endif
#if IMGUI_ADDON_HOTKEY
#include "HotKey.h"
#endif
#if IMGUI_ADDON_ZMO
#include "ImGuizmo.h"
#endif
#if IMGUI_ADDON_DATE_CHOOSER || IMGUI_ADDON_KNOB || IMGUI_ADDON_VARIOUS || IMGUI_ADDON_DOCK || IMGUI_ADDON_TABWINDOW || IMGUI_ADDON_PROGRESSES || IMGUI_ADDON_TIMELINE || IMGUI_VULKAN_SHADER
#include "addon/addons_demo.h"
#endif
#include <stdio.h>
#include <SDL.h>
#include <fstream>
#include <sstream>
#include <string>
#include <cerrno>
#include "Config.h"

#if IMGUI_ADDON_MARKDOWN
static std::string get_file_contents(const char *filename)
{
    std::string file_path = std::string(DEFAULT_DOCUMENT_PATH) + std::string(filename);
    std::ifstream infile(file_path, std::ios::in | std::ios::binary);
    if (infile.is_open())
    {
        std::ostringstream contents;
        contents << infile.rdbuf();
        infile.close();
        return(contents.str());
    }
    throw(errno);
}

inline ImGui::MarkdownImageData ImageCallback( ImGui::MarkdownLinkCallbackData data_ )
{
    // In your application you would load an image based on data_ input. Here we just use the imgui font texture.
    ImTextureID image = ImGui::GetIO().Fonts->TexID;
    // > C++14 can use ImGui::MarkdownImageData imageData{ true, false, image, ImVec2( 40.0f, 20.0f ) };
    ImGui::MarkdownImageData imageData;
    imageData.isValid =         true;
    imageData.useLinkCallback = false;
    imageData.user_texture_id = image;
    imageData.size =            ImVec2( 40.0f, 20.0f );
    return imageData;
}

static void LinkCallback( ImGui::MarkdownLinkCallbackData data_ )
{
    std::string url( data_.link, data_.linkLength );
    std::string command = "open " + url;
    if( !data_.isImage )
    {
        system(command.c_str());
    }
}

static void ExampleMarkdownFormatCallback( const ImGui::MarkdownFormatInfo& markdownFormatInfo_, bool start_ )
{
    // Call the default first so any settings can be overwritten by our implementation.
    // Alternatively could be called or not called in a switch statement on a case by case basis.
    // See defaultMarkdownFormatCallback definition for furhter examples of how to use it.
    ImGui::defaultMarkdownFormatCallback( markdownFormatInfo_, start_ );        
    switch( markdownFormatInfo_.type )
    {
        // example: change the colour of heading level 2
        case ImGui::MarkdownFormatType::HEADING:
        {
            if( markdownFormatInfo_.level == 2 )
            {
                if( start_ )
                {
                    ImGui::PushStyleColor( ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled] );
                }
                else
                {
                    ImGui::PopStyleColor();
                }
            }
            break;
        }
        default:
        {
            break;
        }
    }
}
#endif

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
// About Desktop OpenGL function loaders:
//  Modern desktop OpenGL doesn't have a standard portable header file to load OpenGL function pointers.
//  Helper libraries are often used for this purpose! Here we are supporting a few common ones (gl3w, glew, glad).
//  You may use another loader/header of your choice (glext, glLoadGen, etc.), or chose to manually implement your own.
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h>            // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h>            // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h>          // Initialize with gladLoadGL()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD2)
#include <glad/gl.h>            // Initialize with gladLoadGL(...) or gladLoaderLoadGL()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
#define GLFW_INCLUDE_NONE       // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/Binding.h>  // Initialize with glbinding::Binding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
#define GLFW_INCLUDE_NONE       // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/glbinding.h>// Initialize with glbinding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif

// Main code
int main(int, char**)
{
    // Setup SDL
    // (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
    // depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }
    int window_width = 1440;
    int window_height = 960;
    float window_scale = 1.0;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    bool err = gladLoadGL() == 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD2)
    bool err = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress) == 0; // glad2 recommend using the windowing library loader instead of the (optionally) bundled one.
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
    bool err = false;
    glbinding::Binding::initialize();
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
    bool err = false;
    glbinding::initialize([](const char* name) { return (glbinding::ProcAddress)SDL_GL_GetProcAddress(name); });
#else
    bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
#if IMGUI_ADDON_IMPLOTS
    ImPlot::CreateContext();
#endif
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    std::string ini_file = std::string(DEFAULT_CONFIG_PATH) + "sdl_opengl3.ini";
    io.IniFilename = ini_file.c_str();
    io.FontGlobalScale = window_scale;

    //io.Fonts->AddFontDefault();
    
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

#if IMGUI_ADDON_FILE_DIALOG
    // load file dialog resource
    ImGuiFileDialog filedialog;
    std::string bookmark_path = std::string(DEFAULT_CONFIG_PATH) + "bookmark.ini";
    prepare_file_dialog_demo_window(&filedialog, bookmark_path.c_str());
#endif
#if IMGUI_ADDON_FILE_SYSTEM
    // init sample file dialog
    ImGuiFs::Dialog dlg;
#endif
#if IMGUI_ADDON_MEMORY_EDITOR
    // init memory edit
    MemoryEditor mem_edit;
    mem_edit.Open = false;
    mem_edit.OptShowDataPreview = true;
    size_t data_size = 0x1000;
    void* data = malloc(data_size);
#endif
#if IMGUI_ADDON_TEXT_EDITOR
    // Init Text Edit
	TextEditor editor;
#endif
#if IMGUI_ADDON_MARKDOWN
    // Init MarkDown
    ImGui::MarkdownConfig mdConfig; 
#endif
#if IMGUI_ADDON_IMNODES
    // Init imnodes
    std::string node_ini_path = std::string(DEFAULT_CONFIG_PATH) + "nodes_save_load.ini";
    std::string node_path = std::string(DEFAULT_CONFIG_PATH) + "nodes_save_load.node";
    ImNodes::CreateContext();
    imnodes_example::NodeEditorInitialize(node_ini_path.c_str(), node_path.c_str());
#endif
#if IMGUI_ADDON_NODE_GRAPH
    // Init NodeGraphEditor
    ImGui::NodeGraphEditor nge;
    std::string nge_ini_path = std::string(DEFAULT_CONFIG_PATH) + "nodeGraphEditor.nge.ini";
    std::string nge_style_path = std::string(DEFAULT_CONFIG_PATH) + "nodeGraphEditor.style.ini";
    nge.save_node_path = nge_ini_path;
    nge.save_style_path = nge_style_path;
#endif
#if IMGUI_ADDON_HOTKEY
    // Init HotKey
    static std::vector<ImHotKey::HotKey> hotkeys = 
    { 
        {"Layout", "Reorder nodes in a simpler layout", 0xFFFF26E0},
        {"Save", "Save the current graph", 0xFFFF1FE0},
        {"Load", "Load an existing graph file", 0xFFFF18E0},
        {"Play/Stop", "Play or stop the animation from the current graph", 0xFFFFFF3F},
        {"SetKey", "Make a new animation key with the current parameters values at the current time", 0xFFFFFF1F}
    };
#endif

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;

    bool show_implot_window = false;
    bool show_file_dialog_window = false;
    bool show_sample_file_dialog = false;
    bool show_text_edit_window = false;
    bool show_markdown_window = false;
    bool show_dock_window = false;
    bool show_tab_window = false;
    bool show_node_window = false;
    bool show_node_edit_window = false;
    bool show_addon_widget = false;
    bool show_zmo_window = false;

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
    bool show = true;
    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
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
        }
        if (!show)
        {
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);
#if IMGUI_ADDON_IMPLOTS
            ImGui::Checkbox("ImPlot Window", &show_implot_window);
#endif
#if IMGUI_ADDON_FILE_DIALOG
            ImGui::Checkbox("File Dialog Window", &show_file_dialog_window);
#endif
#if IMGUI_ADDON_FILE_SYSTEM
            ImGui::Checkbox("Sample File Dialog", &show_sample_file_dialog);
#endif
#if IMGUI_ADDON_MEMORY_EDITOR
            ImGui::Checkbox("Memory Edit Window", &mem_edit.Open);
#endif
#if IMGUI_ADDON_TEXT_EDITOR
            ImGui::Checkbox("Show Text Edit Window", &show_text_edit_window);
#endif
#if IMGUI_ADDON_MARKDOWN
            ImGui::Checkbox("Show Markdown Window", &show_markdown_window);
#endif
#if IMGUI_ADDON_DOCK
            ImGui::Checkbox("Show Dock Window", &show_dock_window);
#endif
#if IMGUI_ADDON_TABWINDOW
            ImGui::Checkbox("Show Tab Window", &show_tab_window);
#endif
#if IMGUI_ADDON_IMNODES
            ImGui::Checkbox("Show Node Sample Window", &show_node_window);
#endif
#if IMGUI_ADDON_NODE_GRAPH
            ImGui::Checkbox("Show Node Edit Windows", &show_node_edit_window);
#endif
#if IMGUI_ADDON_DATE_CHOOSER || IMGUI_ADDON_KNOB || IMGUI_ADDON_VARIOUS || IMGUI_ADDON_DOCK || IMGUI_ADDON_TABWINDOW || IMGUI_ADDON_PROGRESSES || IMGUI_ADDON_TIMELINE
            ImGui::Checkbox("Show Addon Widgets", &show_addon_widget);
#endif
#if IMGUI_ADDON_ZMO
            ImGui::Checkbox("Show ImGuizmo Window", &show_zmo_window);
#endif
#if IMGUI_ADDON_HOTKEY
            // show hotkey window
            if (ImGui::Button("Edit Hotkeys"))
            {
                ImGui::OpenPopup("HotKeys Editor");
            }

            // Handle hotkey popup
            ImHotKey::Edit(hotkeys.data(), hotkeys.size(), "HotKeys Editor");
            int hotkey = ImHotKey::GetHotKey(hotkeys.data(), hotkeys.size());
            if (hotkey != -1)
            {
                // handle the hotkey index!
            }
#endif
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        // Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }
#if IMGUI_ADDON_IMPLOTS
        // Show ImPlot simple window
        if (show_implot_window)
        {
            ImPlot::ShowDemoWindow(&show_implot_window);
        }
#endif

#if IMGUI_ADDON_FILE_DIALOG
        // Show FileDialog demo window
        if (show_file_dialog_window)
        {
            show_file_dialog_demo_window(&filedialog, &show_file_dialog_window);
        }
#endif

#if IMGUI_ADDON_FILE_SYSTEM
        // Show Sample FileDialog
        {
            // dlg.WrapMode = false;
            const char* filePath = dlg.chooseFileDialog(show_sample_file_dialog, dlg.getLastDirectory(), ".jpg;.jpeg;.png;.gif;.tga;.bmp", "Sample file dialog", ImVec2(400, 800), ImVec2(50, 50));
            if (strlen(filePath) > 0) 
            {
	            //fprintf(stderr,"Browsed..: %s\n",filePath);
            }
            show_sample_file_dialog = false;
        }
#endif

#if IMGUI_ADDON_MEMORY_EDITOR
        // Show Memory Edit window
        if (mem_edit.Open)
        {
            mem_edit.DrawWindow("Memory Editor", data, data_size, 0, &mem_edit.Open, 768);
        }
#endif

#if IMGUI_ADDON_TEXT_EDITOR
        // Show Text Edit Window
        if (show_text_edit_window)
        {
            editor.text_edit_demo(&show_text_edit_window);
        }
#endif

#if IMGUI_ADDON_MARKDOWN
        // Show Markdown Window
        if (show_markdown_window)
        {
            std::string help_doc = get_file_contents("imgui.md");
            mdConfig.linkCallback =         LinkCallback;
            mdConfig.tooltipCallback =      NULL;
            mdConfig.imageCallback =        ImageCallback;
            mdConfig.linkIcon =             ICON_FA5_LINK;
            mdConfig.headingFormats[0] =    { io.Fonts->Fonts[0], true };
            mdConfig.headingFormats[1] =    { io.Fonts->Fonts.size() > 1 ? io.Fonts->Fonts[1] : nullptr, true };
            mdConfig.headingFormats[2] =    { io.Fonts->Fonts.size() > 2 ? io.Fonts->Fonts[2] : nullptr, false };
            mdConfig.userData =             NULL;
            mdConfig.formatCallback =       ExampleMarkdownFormatCallback;
            ImGui::Markdown( help_doc.c_str(), help_doc.length(), mdConfig );
        }
#endif

#if IMGUI_ADDON_DOCK
        // Show Dock Window
        if (show_dock_window)
        {
            ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
            if(ImGui::Begin("imguidock window (= lumix engine's dock system)",&show_dock_window, ImGuiWindowFlags_NoScrollbar))
            {
                ImGui::ShowAddonsDuckWindow();
            }
            ImGui::End();
        }
#endif

#if IMGUI_ADDON_TABWINDOW
        // Show Tab Window
        if (show_tab_window)
        {
            ImGui::SetNextWindowSize(ImVec2(700,600), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Example: TabWindow", &show_tab_window, ImGuiWindowFlags_NoScrollbar))
            {
                ImGui::ShowAddonsTabWindow();   // see its code for further info         
            }
            ImGui::End();
        }
#endif

#if IMGUI_ADDON_IMNODES
        // Show Node Window
        if (show_node_window)
        {
            imnodes_example::NodeEditorShow();
        }
#endif

#if IMGUI_ADDON_NODE_GRAPH
        // Show Node Edit Window
        if (show_node_edit_window)
        {
            ImGui::SetNextWindowSize(ImVec2(700,600), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Example: Custom Node Graph",&show_node_edit_window, ImGuiWindowFlags_NoScrollbar))
            {
                std::string node_ini_path = std::string(DEFAULT_CONFIG_PATH) + "nodeGraphEditor.nge.ini";
                std::string node_style_path = std::string(DEFAULT_CONFIG_PATH) + "nodeGraphEditor.style.ini";
                ImGui::TestNodeGraphEditor(&nge);   // see its code for further info         
            }
            ImGui::End();
        }
#endif

#if IMGUI_ADDON_DATE_CHOOSER || IMGUI_ADDON_KNOB || IMGUI_ADDON_VARIOUS || IMGUI_ADDON_DOCK || IMGUI_ADDON_TABWINDOW || IMGUI_ADDON_PROGRESSES || IMGUI_ADDON_TIMELINE
        // Show Addon Widget.
        if (show_addon_widget)
        {
            ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
            ImGui::Begin("Addon Widget", &show_addon_widget);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::ShowAddonsDemoWindowWidgets();
            ImGui::End();
        }
#endif

#if IMGUI_ADDON_ZMO
        // Show Zmo Window
        if (show_zmo_window)
        {
            ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(1280, 800), ImGuiCond_FirstUseEver);
            ImGui::Begin("##ZMO", &show_zmo_window, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
            ImGuizmo::ShowAddonsZMOWindow();
            ImGui::End();
        }
#endif
        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

#if IMGUI_ADDON_MEMORY_EDITOR
    // Cleanup memory edit resource
    if (data)
        free(data);
#endif

#if IMGUI_ADDON_FILE_DIALOG
    // Store file dialog bookmark
    end_file_dialog_demo_window(&filedialog, bookmark_path.c_str());
#endif

#if IMGUI_ADDON_IMNODES
    // Clean Node Window
    imnodes_example::NodeEditorShutdown(node_ini_path.c_str(), node_path.c_str());
    ImNodes::DestroyContext();
#endif

#if IMGUI_ADDON_DATE_CHOOSER || IMGUI_ADDON_KNOB || IMGUI_ADDON_VARIOUS || IMGUI_ADDON_DOCK || IMGUI_ADDON_TABWINDOW || IMGUI_ADDON_PROGRESSES || IMGUI_ADDON_TIMELINE
    // Cleanup Demo
    ImGui::CleanupDemo();
#endif

#if IMGUI_ADDON_ZMO
    ImGuizmo::CleanupZMODemo();
#endif

#if IMGUI_ADDON_NODE_GRAPH
    nge.clear();
#endif

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
#if IMGUI_ADDON_IMPLOTS
    ImPlot::DestroyContext();
#endif
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
