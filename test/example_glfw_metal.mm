// Dear ImGui: standalone example application for GLFW + Metal, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_metal.h"
#ifdef IMGUI_ADDONS
#include "implot.h"
#include "imgui_markdown.h"
#include "imgui_memory_editor.h"
#ifdef IMGUI_ADDONS_IMNODES
#include "imnodes.h"
#endif
#ifdef IMGUI_ADDONS_NODE_GRAPH
#include "ImGuiNodeGraphEditor.h"
#endif
#include "TextEditor.h"
#include "ImGuiFileDialog.h"
#include "ImGuiFileSystem.h"
#include "imgui_dock.h"
#include "HotKey.h"
#include "addon/addons_demo.h"
#endif
#include <stdio.h>
#include <fstream>
#include <sstream>

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
#include "Config.h"

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

#ifdef IMGUI_ADDONS
static std::string get_file_contents(const char *filename)
{
    std::ifstream infile(filename, std::ios::in | std::ios::binary);
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

int main(int, char**)
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
#ifdef IMGUI_ADDONS
    ImPlot::CreateContext();
#endif
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    std::string ini_file = std::string(DEFAULT_CONFIG_PATH) + "glfw_metal.ini";
    io.IniFilename = ini_file.c_str();
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

    // Setup style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.txt' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Create window with graphics context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(1440, 1024, "Dear ImGui GLFW+Metal example", NULL, NULL);
    if (window == NULL)
        return 1;

    id <MTLDevice> device = MTLCreateSystemDefaultDevice();;
    id <MTLCommandQueue> commandQueue = [device newCommandQueue];

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplMetal_Init(device);

    NSWindow *nswin = glfwGetCocoaWindow(window);
    CAMetalLayer *layer = [CAMetalLayer layer];
    layer.device = device;
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    nswin.contentView.layer = layer;
    nswin.contentView.wantsLayer = YES;

    MTLRenderPassDescriptor *renderPassDescriptor = [MTLRenderPassDescriptor new];

#ifdef IMGUI_ADDONS
    // load file dialog resource
    ImGuiFileDialog filedialog;
    std::string bookmark_path = std::string(DEFAULT_CONFIG_PATH) + "bookmark.ini";
    prepare_file_dialog_demo_window(&filedialog, bookmark_path.c_str());

    // init sample file dialog
    ImGuiFs::Dialog dlg;

    // init memory edit
    MemoryEditor mem_edit;
    mem_edit.Open = false;
    mem_edit.OptShowDataPreview = true;
    size_t data_size = 0x1000;
    void* data = malloc(data_size);

    // Init Text Edit
	TextEditor editor;

    // Init MarkDown
    ImGui::MarkdownConfig mdConfig; 

#ifdef IMGUI_ADDONS_IMNODES
    // Init imnodes
    std::string node_ini_path = std::string(DEFAULT_CONFIG_PATH) + "nodes_save_load.ini";
    std::string node_path = std::string(DEFAULT_CONFIG_PATH) + "nodes_save_load.node";
    ImNodes::CreateContext();
    imnodes_example::NodeEditorInitialize(node_ini_path.c_str(), node_path.c_str());
#endif

#ifdef IMGUI_ADDONS_NODE_GRAPH
    // Init NodeGraphEditor
    ImGui::NodeGraphEditor nge;
    std::string nge_ini_path = std::string(DEFAULT_CONFIG_PATH) + "nodeGraphEditor.nge.ini";
    std::string nge_style_path = std::string(DEFAULT_CONFIG_PATH) + "nodeGraphEditor.style.ini";
    nge.save_node_path = nge_ini_path;
    nge.save_style_path = nge_style_path;
#endif

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
#ifdef IMGUI_ADDONS
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
#endif
    float clear_color[4] = {0.45f, 0.55f, 0.60f, 1.00f};

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        @autoreleasepool
        {
            // Poll and handle events (inputs, window resize, etc.)
            // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
            // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
            // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
            // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
            glfwPollEvents();

            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            layer.drawableSize = CGSizeMake(width, height);
            id<CAMetalDrawable> drawable = [layer nextDrawable];

            id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
            renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(clear_color[0] * clear_color[3], clear_color[1] * clear_color[3], clear_color[2] * clear_color[3], clear_color[3]);
            renderPassDescriptor.colorAttachments[0].texture = drawable.texture;
            renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
            renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
            id <MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
            [renderEncoder pushDebugGroup:@"ImGui demo"];

            // Start the Dear ImGui frame
            ImGui_ImplMetal_NewFrame(renderPassDescriptor);
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
            if (show_demo_window)
                ImGui::ShowDemoWindow(&show_demo_window);

            // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
            {
                static float f = 0.0f;
                static int counter = 0;

                ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

                ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
                ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
                ImGui::Checkbox("Another Window", &show_another_window);
#ifdef IMGUI_ADDONS
                ImGui::Checkbox("ImPlot Window", &show_implot_window);
                ImGui::Checkbox("File Dialog Window", &show_file_dialog_window);
                ImGui::Checkbox("Sample File Dialog", &show_sample_file_dialog);
                ImGui::Checkbox("Memory Edit Window", &mem_edit.Open);
                ImGui::Checkbox("Show Text Edit Window", &show_text_edit_window);
                ImGui::Checkbox("Show Markdown Window", &show_markdown_window);
                ImGui::Checkbox("Show Dock Window", &show_dock_window);
                ImGui::Checkbox("Show Tab Window", &show_tab_window);
#ifdef IMGUI_ADDONS_IMNODES
                ImGui::Checkbox("Show Node Sample Window", &show_node_window);
#endif
#ifdef IMGUI_ADDONS_NODE_GRAPH
                ImGui::Checkbox("Show Node Edit Windows", &show_node_edit_window);
#endif
                ImGui::Checkbox("Show Addon Widgets", &show_addon_widget);
                ImGui::Checkbox("Show ImGuizmo Window", &show_zmo_window);

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

            // 3. Show another simple window.
            if (show_another_window)
            {
                ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
                ImGui::Text("Hello from another window!");
                if (ImGui::Button("Close Me"))
                    show_another_window = false;
                ImGui::End();
            }
#ifdef IMGUI_ADDONS
            // 4. Show ImPlot simple window
            if (show_implot_window)
            {
                ImPlot::ShowDemoWindow(&show_implot_window);
            }

            // 5. Show FileDialog demo window
            if (show_file_dialog_window)
            {
                show_file_dialog_demo_window(&filedialog, &show_file_dialog_window);
            }

            // 6. Show Sample FileDialog
            {
                // dlg.WrapMode = false;
                const char* filePath = dlg.chooseFileDialog(show_sample_file_dialog, dlg.getLastDirectory(), ".jpg;.jpeg;.png;.gif;.tga;.bmp", "Sample file dialog", ImVec2(400, 800), ImVec2(50, 50));
                if (strlen(filePath) > 0) 
                {
	                //fprintf(stderr,"Browsed..: %s\n",filePath);
                }
                show_sample_file_dialog = false;
            }

            // 7. Show Memory Edit window
            if (mem_edit.Open)
            {
                ImGui::Begin("Memory Window", &mem_edit.Open);
                mem_edit.DrawWindow("Memory Editor", data, data_size);
                ImGui::End();
            }

            // 8. Show Text Edit Window
            if (show_text_edit_window)
            {
                editor.text_edit_demo(&show_text_edit_window);
            }

            // 9. Show Markdown Window
            if (show_markdown_window)
            {
                std::string help_doc = get_file_contents("docs/imgui.md");
                mdConfig.linkCallback =         LinkCallback;
                mdConfig.tooltipCallback =      NULL;
                mdConfig.imageCallback =        ImageCallback;
                mdConfig.linkIcon =             ICON_FA5_LINK;
                mdConfig.headingFormats[0] =    { io.Fonts->Fonts[0], true };
                mdConfig.headingFormats[1] =    { io.Fonts->Fonts[1], true };
                mdConfig.headingFormats[2] =    { io.Fonts->Fonts[2], false };
                mdConfig.userData =             NULL;
                mdConfig.formatCallback =       ExampleMarkdownFormatCallback;
                ImGui::Markdown( help_doc.c_str(), help_doc.length(), mdConfig );
            }

            // 10. Show Dock Window
            if (show_dock_window)
            {
                ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
                if(ImGui::Begin("imguidock window (= lumix engine's dock system)",&show_dock_window, ImGuiWindowFlags_NoScrollbar))
                {
                    ImGui::ShowAddonsDuckWindow();
                }
                ImGui::End();
            }

            // 11. Show Tab Window
            if (show_tab_window)
            {
                ImGui::SetNextWindowSize(ImVec2(700,600), ImGuiCond_FirstUseEver);
                if (ImGui::Begin("Example: TabWindow", &show_tab_window, ImGuiWindowFlags_NoScrollbar))
                {
                    ImGui::ShowAddonsTabWindow();   // see its code for further info         
                }
                ImGui::End();
            }
#ifdef IMGUI_ADDONS_IMNODES
            // 12. Show Node  Window
            if (show_node_window)
            {
                imnodes_example::NodeEditorShow();
            }
#endif
#ifdef IMGUI_ADDONS_NODE_GRAPH
            // 13. Show Node Edit Window
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

            // 14. Show Addon Widget.
            if (show_addon_widget)
            {
                ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
                ImGui::Begin("Addon Widget", &show_addon_widget);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
                ImGui::ShowAddonsDemoWindowWidgets();
                ImGui::End();
            }

            // 15. Show Zmo Window
            if (show_zmo_window)
            {
                ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
                ImGui::SetNextWindowSize(ImVec2(1280, 800), ImGuiCond_FirstUseEver);
                ImGui::Begin("##ZMO", &show_zmo_window, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
                ImGui::ShowAddonsZMOWindow();
                ImGui::End();
            }
#endif
            // Rendering
            ImGui::Render();
            ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandBuffer, renderEncoder);

            [renderEncoder popDebugGroup];
            [renderEncoder endEncoding];

            [commandBuffer presentDrawable:drawable];
            [commandBuffer commit];
        }
    }

#ifdef IMGUI_ADDONS
    // Cleanup memory edit resource
    if (data)
        free(data);

    // Store file dialog bookmark
    end_file_dialog_demo_window(&filedialog, bookmark_path.c_str());

#ifdef IMGUI_ADDONS_IMNODES
    // Clean Node Window
    imnodes_example::NodeEditorShutdown(node_ini_path.c_str(), node_path.c_str());
    ImNodes::DestroyContext();
#endif

    // Cleanup Demo
    ImGui::CleanupDemo();
    ImGui::CleanupZMODemo();
#ifdef IMGUI_ADDONS_NODE_GRAPH
    nge.clear();
#endif
#endif
    // Cleanup
    ImGui_ImplMetal_Shutdown();
    ImGui_ImplGlfw_Shutdown();
#ifdef IMGUI_ADDONS
    ImPlot::DestroyContext();
#endif
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
