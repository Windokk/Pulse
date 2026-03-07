#include "main_window.hpp"

#include "engine/core/engine.hpp"

#include "editor/gui/panels/asset_browser/asset_browser.hpp"

#include <fstream>
#include <vector>
#include <string>
#include <cstdlib> // for malloc, free
#include <cstring> // for memcpy

#include "editor/gui/IconsLucide.h"

namespace Pulse::Editor::Core{
    
    void SetupImGuiStyle()
    {
        // Pulse style from ImThemes
        ImGuiStyle& style = ImGui::GetStyle();
        
        style.Alpha = 1.0f;
        style.DisabledAlpha = 0.6f;
        style.WindowPadding = ImVec2(8.0f, 8.0f);
        style.WindowRounding = 0.0f;
        style.WindowBorderSize = 1.0f;
        style.WindowMinSize = ImVec2(32.0f, 32.0f);
        style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
        style.WindowMenuButtonPosition = ImGuiDir_Left;
        style.ChildRounding = 0.0f;
        style.ChildBorderSize = 1.0f;
        style.PopupRounding = 0.0f;
        style.PopupBorderSize = 1.0f;
        style.FramePadding = ImVec2(4.0f, 4.0f);
        style.FrameRounding = 0.0f;
        style.FrameBorderSize = 0.0f;
        style.ItemSpacing = ImVec2(8.0f, 4.0f);
        style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
        style.CellPadding = ImVec2(4.0f, 2.0f);
        style.IndentSpacing = 21.0f;
        style.ColumnsMinSpacing = 6.0f;
        style.ScrollbarSize = 14.0f;
        style.ScrollbarRounding = 9.0f;
        style.GrabMinSize = 10.0f;
        style.GrabRounding = 0.0f;
        style.TabRounding = 4.0f;
        style.TabBorderSize = 0.0f;
        style.ColorButtonPosition = ImGuiDir_Right;
        style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
        style.SelectableTextAlign = ImVec2(0.5f, 0.0f);
        
        ImVec4* colors = style.Colors;
        colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg]               = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
        colors[ImGuiCol_ChildBg]                = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
        colors[ImGuiCol_Border]                 = ImVec4(0.36f, 0.43f, 0.53f, 0.50f);
        colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg]                = ImVec4(0.20f, 0.21f, 0.22f, 0.54f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.33f, 0.49f, 0.60f, 0.62f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.18f, 0.18f, 0.18f, 0.67f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_CheckMark]              = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.34f, 0.42f, 0.48f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.33f, 0.49f, 0.60f, 1.00f);
        colors[ImGuiCol_Button]                 = ImVec4(0.44f, 0.44f, 0.44f, 0.40f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.33f, 0.49f, 0.60f, 1.00f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.42f, 0.42f, 0.42f, 1.00f);
        colors[ImGuiCol_Header]                 = ImVec4(0.35f, 0.35f, 0.35f, 0.31f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.25f, 0.34f, 0.40f, 0.80f);
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.48f, 0.50f, 0.52f, 1.00f);
        colors[ImGuiCol_Separator]              = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
        colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.72f, 0.72f, 0.72f, 0.78f);
        colors[ImGuiCol_SeparatorActive]        = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
        colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
        colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.46f, 0.46f, 0.46f, 0.95f);
        colors[ImGuiCol_InputTextCursor]        = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TabHovered]             = ImVec4(0.28f, 0.34f, 0.40f, 1.00f);
        colors[ImGuiCol_Tab]                    = ImVec4(0.20f, 0.26f, 0.31f, 0.86f);
        colors[ImGuiCol_TabSelected]            = ImVec4(0.33f, 0.49f, 0.60f, 1.00f);
        colors[ImGuiCol_TabSelectedOverline]    = ImVec4(0.31f, 0.37f, 0.45f, 1.00f);
        colors[ImGuiCol_TabDimmed]              = ImVec4(0.15f, 0.20f, 0.25f, 0.97f);
        colors[ImGuiCol_TabDimmedSelected]      = ImVec4(0.23f, 0.26f, 0.30f, 1.00f);
        colors[ImGuiCol_TabDimmedSelectedOverline]  = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
        colors[ImGuiCol_DockingPreview]         = ImVec4(0.20f, 0.26f, 0.31f, 0.86f);
        colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_PlotLines]              = ImVec4(0.53f, 0.53f, 0.53f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered]       = ImVec4(0.65f, 0.79f, 0.88f, 1.00f);
        colors[ImGuiCol_PlotHistogram]          = ImVec4(0.36f, 0.57f, 0.70f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(0.61f, 0.79f, 0.90f, 1.00f);
        colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
        colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.41f, 0.47f, 0.50f, 1.00f);
        colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
        colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        colors[ImGuiCol_TextLink]               = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.87f, 0.87f, 0.87f, 0.35f);
        colors[ImGuiCol_TreeLines]              = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
        colors[ImGuiCol_DragDropTarget]         = ImVec4(0.33f, 0.49f, 0.60f, 1.00f);
        colors[ImGuiCol_DragDropTargetBg]       = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_UnsavedMarker]          = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_NavCursor]              = ImVec4(0.39f, 0.61f, 0.78f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.7f);
        colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    }

    ImFont* LoadFontFromFile(const std::string& filePath, float fontSize, bool icons)
    {
        // Open the file in binary mode
        std::ifstream fontFile(filePath, std::ios::binary | std::ios::ate);
        if (!fontFile.is_open())
        {
            DEBUG_ERROR("Failed to load editor font: %s\n", filePath.c_str());
            return nullptr;
        }

        // Get file size and read content
        std::streamsize size = fontFile.tellg();
        fontFile.seekg(0, std::ios::beg);

        if (size <= 0)
        {
            DEBUG_ERROR("Failed to load editor font (empty file): %s\n", filePath.c_str());
            return nullptr;
        }

        std::vector<unsigned char> fontData(size);
        if (!fontFile.read(reinterpret_cast<char*>(fontData.data()), size))
        {
            DEBUG_ERROR("Failed to read editor font: %s\n", filePath.c_str());
            return nullptr;
        }

        fontFile.close();

        // Allocate memory ImGui can take ownership of
        void* fontMemory = malloc(fontData.size());
        if (!fontMemory)
            return nullptr;

        memcpy(fontMemory, fontData.data(), fontData.size());

        ImGuiIO& io = ImGui::GetIO();

        ImFontConfig cfg{};
        cfg.MergeMode = icons;
        cfg.FontDataOwnedByAtlas = true;
        cfg.PixelSnapH = true;
        cfg.GlyphOffset.y = icons ? 3.0f : 0.0f;

        const ImWchar icons_ranges[] = { ICON_MIN_LC, ICON_MAX_16_LC, 0 };

        ImFont* font = nullptr;
        if (icons)
        {
            font = io.Fonts->AddFontFromMemoryTTF(
                fontMemory,
                static_cast<int>(fontData.size()),
                fontSize,
                &cfg,
                icons_ranges
            );
        }
        else
        {
            font = io.Fonts->AddFontFromMemoryTTF(
                fontMemory,
                static_cast<int>(fontData.size()),
                fontSize,
                &cfg,
                io.Fonts->GetGlyphRangesDefault()
            );
        }

        if (!font)
        {
            DEBUG_ERROR("Failed to load editor font from ImGui memory: %s\n", filePath.c_str());
            free(fontMemory);
        }

        return font;
    }

    void EditorMainWindow::Init(const std::string &title, const int &width, const int &height, const bool &fullscreen, const int &vsync)
    {
        //Init glfw and gl context
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_SAMPLES, 4);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        if(fullscreen){
            GLFWmonitor* primary = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(primary);
            window = glfwCreateWindow(mode->width, mode->height, title.c_str(), primary, nullptr);
        }
        else{
            window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
        }
        
        if (window == NULL)
        {
            glfwTerminate();
            DEBUG_FATAL("Failed to create GLFW window");
        }

        glfwMakeContextCurrent(window);
        glfwSetWindowUserPointer(window, this);
        glfwSwapInterval(vsync);

        gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

        OpenGL* gl = new OpenGL();
        gl->InitFromGLAD();

        Engine::Core::GetEngine().SetGL(gl);
    }

    void EditorMainWindow::SetGLFWInputManager(GLFWInput *inputManager)
    {
        this->inputManager = inputManager;
    }

    void EditorMainWindow::SetTitle(const std::string &title)
    {
        glfwSetWindowTitle(window, title.c_str());
    }

    void EditorMainWindow::PollEvents()
    {
        glfwPollEvents();
    }

    void EditorMainWindow::SwapBuffers()
    {
        if(!imguiInitialized)
        {
            //Init ImGui
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
            io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
            io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
            io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;

            // Setup Platform/Renderer backends
            ImGui_ImplGlfw_InitForOpenGL(window, true);
            ImGui_ImplOpenGL3_Init();
            
            SetupImGuiStyle();

            LoadFontFromFile("editor_resources/fonts/OpenSans-Regular.ttf", 16, false);
            LoadFontFromFile("editor_resources/fonts/lucide.ttf", 16, true);

            //Init Panels
            assetBrowser = new GUI::AssetBrowser();
            assetBrowser->NavigateTo(Engine::Core::GetEngine().GetCurrentProject()->GetProjectResourcesPath().full);
            propertiesPanel = new GUI::PropertiesPanel();
            levelTree = new GUI::LevelTree();
            levelTree->SetParentWindow(this);
            viewport = new GUI::ViewportWindow();
            viewport->SetParentWindow(this);

            GUI::EditorResources::Instance().Init();

            imguiInitialized = true;
        }

        // Wait for viewport size to be initialized
        if(!renderPassesInitialized && viewport->GetViewportSize() != glm::vec2(50,50)){
            
            Rendering::Renderer* renderer = Engine::Core::GetEngine().GetRenderer();

            auto fbOutlineShader = Engine::Core::GetEngine().GetResourcesManager()->GetShader("shaders/editor/outline");

            Rendering::FrameBuffer fbOutline(
                viewport->GetViewportSize().x,
                viewport->GetViewportSize().y,
                fbOutlineShader,
                false
            );

            fbOutlineShader->Activate();

            fbOutlineShader->SetInt("maskTex", 0);
            fbOutlineShader->SetFloat("outlineThickness", 4.0f);
            fbOutlineShader->SetVec2("texelSize", glm::vec2(1.0 / fbOutline.width, 1.0 / fbOutline.height));
            fbOutlineShader->SetVec3("outlineColor", glm::vec3(1.0f, 0.722f, 0.0f));

            fbOutlineShader->Deactivate();

            // Init Editor Render Pass
            auto drawOutlineMaskFunc = [this, renderer, fbOutlineShader]() {

                if(!selectedActor || !settings.showOutlines)
                    return;

                OpenGL* gl = Engine::Core::GetEngine().GetGL();

                gl->ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                gl->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                gl->Disable(GL_DEPTH_TEST);
                gl->Enable(GL_CULL_FACE);
                gl->CullFace(GL_BACK);
                gl->FrontFace(GL_CCW);

                for(auto& cmd : *renderer->GetDrawList()){
                    
                    if(cmd.tr->parent->GetID() != selectedActor->GetID())
                        continue;

                    Engine::Rendering::CameraManager* camMan = Engine::Core::GetEngine().GetCameraManager();

                    std::shared_ptr<Engine::Rendering::Shader> shader = renderer->defaultUnlitShader;

                    shader->Activate();

                    shader->SetMat4("projection", camMan->GetActiveCamera()->GetProjection());
                    shader->SetMat4("view", camMan->GetActiveCamera()->GetView());
                    shader->SetMat4("model", cmd.tr->GetTransformMatrix());
                    
                    shader->SetBool("masked", false);
                    shader->SetBool("useTexture", false);
                    shader->SetBool("useCustomColor", true);
                    shader->SetVec4("customColor", glm::vec4(1));

                    gl->BindVertexArray(cmd.VAO);
                    gl->PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                    gl->DrawElements(GL_TRIANGLES, cmd.indexCount, GL_UNSIGNED_INT, (void*)(cmd.indexOffset * sizeof(uint32_t)));

                    shader->Deactivate();
                }

                gl->BindVertexArray(0);
                gl->UseProgram(0);
            };

            renderer->AddRenderPass(
                Rendering::RenderPassType::Fullscreen,
                drawOutlineMaskFunc,
                std::make_shared<Rendering::FrameBuffer>(fbOutline),
                true,
                Rendering::BlendMode::Normal
            );

            renderPassesInitialized = true;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();


        ImGuizmo::BeginFrame();
        
        ImGui::DockSpaceOverViewport();

        assetBrowser->Draw();
        viewport->Draw();
        propertiesPanel->Draw(selectedActor);
        levelTree->Draw();

        ImGui::ShowDemoWindow();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    bool EditorMainWindow::ShouldClose() const
    {
        return glfwWindowShouldClose(window);
    }

    int EditorMainWindow::GetFramebufferWidth() const
    {
        if(viewport)
            return viewport->GetViewportSize().x;
        return 50;
    }

    int EditorMainWindow::GetFramebufferHeight() const
    {
        if(viewport)
            return viewport->GetViewportSize().y;
        return 50;
    }

    void *EditorMainWindow::GetNativeHandle() const
    {
        return static_cast<void*>(window);
    }

    void EditorMainWindow::Destroy() const
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void EditorMainWindow::ToggleFullscreen()
    {
        const bool fullscreen = glfwGetWindowMonitor(window) != nullptr;
        if(fullscreen) {
            // Restore the window position and size.
            glfwSetWindowMonitor(window, nullptr, windowPosX, windowPosY, windowWidth, windowHeight, 0);
            // Check the window position and size (if we are on a screen smaller than the initial size).
            glfwGetWindowPos(window, &windowPosX, &windowPosY);
            glfwGetWindowSize(window, &windowWidth, &windowHeight);
        } else {
            // Backup the window current frame.
            glfwGetWindowPos(window, &windowPosX, &windowPosY);
            glfwGetWindowSize(window, &windowWidth, &windowHeight);
            // Move to fullscreen on the primary monitor.
            GLFWmonitor * monitor	= glfwGetPrimaryMonitor();
            const GLFWvidmode * mode = glfwGetVideoMode(monitor);
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        }
    }

    void EditorMainWindow::ProcessInputs() const
    {
        if(Engine::Core::GetEngine().IsInPlayMode())
            return;

        Engine::Core::Platform::IInput* input = Engine::Core::GetEngine().GetInputManager();

        if(input->IsKeyDown(Engine::Input::Key::LeftControl)){
        
            if(input->WasKeyPressed(Engine::Input::Key::S)){
                Engine::Core::GetEngine().GetLevelManager()->GetLevelAt(0)->Serialize(
                    Engine::Core::GetEngine().GetLevelManager()->GetLevelAt(0)->GetPath()
                );
                DEBUG_LOG("Successfully saved level");
                return;
            }
            else if(input->WasKeyPressed(Engine::Input::Key::W)){
                Editor::Commands::CommandStack::Get().Undo();
                return;
            }
            else if(input->WasKeyPressed(Engine::Input::Key::Y)){
                Editor::Commands::CommandStack::Get().Redo();
                return;
            }
        }

        if(viewport->IsHovered()){
            viewport->ProcessInputs();
        }
    }

    int EditorMainWindow::GetBytesPerPixel() const
    {
        int redBits = glfwGetWindowAttrib(window, GLFW_RED_BITS);
        int greenBits = glfwGetWindowAttrib(window, GLFW_GREEN_BITS);
        int blueBits = glfwGetWindowAttrib(window, GLFW_BLUE_BITS);
        int alphaBits = glfwGetWindowAttrib(window, GLFW_ALPHA_BITS);

        return (redBits + greenBits + blueBits + alphaBits) / 8;
    }

    Pulse::Engine::Core::Platform::SystemInfos EditorMainWindow::GetSystemInfos() const
    {
        Engine::Core::Platform::SystemInfos ret{};
                
        const GLubyte* vendor   = Engine::Core::GetEngine().GetGL()->GetString(GL_VENDOR);
        const GLubyte* renderer = Engine::Core::GetEngine().GetGL()->GetString(GL_RENDERER);
        const GLubyte* version  = Engine::Core::GetEngine().GetGL()->GetString(GL_VERSION);

        ret.gpu_vendor   = vendor   ? reinterpret_cast<const char*>(vendor)   : "Unknown";
        ret.gpu_renderer = renderer ? reinterpret_cast<const char*>(renderer) : "Unknown";
        ret.gl_version   = version  ? reinterpret_cast<const char*>(version)  : "Unknown";

        // GLFW context version
        int major, minor, rev;
        glfwGetVersion(&major, &minor, &rev);
        ret.windowHostVersion = std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(rev);

        // Monitor and resolution
        int count = 0;
        GLFWmonitor** monitors = glfwGetMonitors(&count);
        ret.connectedMonitorsCount = count;

        for (int i = 0; i < count; ++i) {
            const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);
            ret.monitors.push_back({mode->width, mode->height, mode->refreshRate});
        }
        
        ret.windowHost = Engine::Core::Platform::GLFW;

        return ret;
    }
}

