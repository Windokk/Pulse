#include "main_window.hpp"

#include "engine/core/engine.hpp"

#include "editor/gui/panels/asset_browser/asset_browser.hpp"

#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <cstring>

#include "editor/gui/IconsLucide.h"

#include "engine/core/resources/resources_manager.hpp"
#include "engine/levels/level_manager.hpp"
#include "engine/projects/project.hpp"
#include "engine/rendering/renderer/renderer.hpp"
#include "engine/rendering/shader/shader.hpp"
#include "engine/rendering/pipeline/pipeline.hpp"
#include "engine/rendering/material/material.hpp"
#include "engine/core/engine.hpp"

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

    void EditorMainWindow::Init(const std::string &title, const int &width, const int &height, const bool &fullscreen, const int &vsync, const uint32_t& api)
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

        if(api == (uint32_t)Engine::Rendering::RendererAPI::API::OpenGL)
            gladLoadGL((GLADloadfunc)glfwGetProcAddress);
        else if(api == (uint32_t)Engine::Rendering::RendererAPI::API::Vulkan)
        {    
            //gladLoadVulkan(Engine::Core::GetEngine().GetRenderer()->GetDevicePointer?,(GLADloadfunc)glfwGetProcAddress);
        }
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
            console = new GUI::Console();
            console->SetParentWindow(this);

            GUI::EditorResources::Instance().Init();

            imguiInitialized = true;
        }
            
        Rendering::Renderer* renderer = Engine::Core::GetEngine().GetRenderer();

        // Wait for viewport size to be initialized
        if(!renderPassesInitialized && viewport->GetViewportSize() != glm::vec2(50,50)){
        
            /// Outline mask pass

            Rendering::FramebufferSpecifications fbOutlineMaskSpecs;
            fbOutlineMaskSpecs.hasColor = true;
            fbOutlineMaskSpecs.hasDepth = true;
            fbOutlineMaskSpecs.colorSpecs = {};
            fbOutlineMaskSpecs.width = viewport->GetViewportSize().x;
            fbOutlineMaskSpecs.height = viewport->GetViewportSize().y;

            std::shared_ptr<Rendering::Framebuffer> fbOutlineMask = Rendering::Framebuffer::Create(fbOutlineMaskSpecs);

            std::shared_ptr<Rendering::Shader> outlineMaskShader = Engine::Core::GetEngine().GetResourcesManager()->GetShader("shaders/editor/outline_mask");
            
            Rendering::PipelineSpecifications outlineMaskPipelineSpecs;
            outlineMaskPipelineSpecs.shader = outlineMaskShader;
            outlineMaskPipelineSpecs.debugName = "OutlineMaskPipeline";
            std::shared_ptr<Rendering::Pipeline> outlineMaskPipeline = Engine::Core::GetEngine().GetRenderer()->GetOrAddPipeline(outlineMaskPipelineSpecs);

            std::shared_ptr<Rendering::Material> outlineMaskMaterial = Rendering::Material::Create(outlineMaskShader, outlineMaskPipeline, false, Rendering::Opacity::Opaque);

            std::shared_ptr<Rendering::RenderPass> outlineMaskPass = std::make_shared<Rendering::RenderPass>();
            outlineMaskPass->clearColor = true;
            outlineMaskPass->clearDepth = true;
            outlineMaskPass->customPipeline = outlineMaskPipeline;
            outlineMaskPass->target = fbOutlineMask;
            outlineMaskPass->overridePipeline = true;
            if(selectedActor)
                outlineMaskPass->customUniforms.emplace("selectedObjID", selectedActor->GetID().GetAsInt());

            renderer->AddRenderPass(outlineMaskPass, "EditorOutlineMaskPass", {});

            /////////

            /// Outline Pass
            
            Rendering::FramebufferSpecifications fbOutlineSpecs;
            fbOutlineSpecs.hasColor = true;
            fbOutlineSpecs.hasDepth = false;
            fbOutlineSpecs.colorSpecs = {};
            fbOutlineSpecs.width = viewport->GetViewportSize().x;
            fbOutlineSpecs.height = viewport->GetViewportSize().y;

            std::shared_ptr<Rendering::Framebuffer> fbOutline = Rendering::Framebuffer::Create(fbOutlineSpecs);

            std::shared_ptr<Rendering::Shader> outlineShader = Engine::Core::GetEngine().GetResourcesManager()->GetShader("shaders/editor/outline");
            
            Rendering::PipelineSpecifications outlinePipelineSpecs;
            outlinePipelineSpecs.depthTest = false;
            outlinePipelineSpecs.depthWrite = false;
            outlinePipelineSpecs.shader = outlineShader;
            outlinePipelineSpecs.debugName = "FullscreenOutlinePipeline";
            outlinePipelineSpecs.vertexLayout = {};
            std::shared_ptr<Rendering::Pipeline> outlinePipeline = Engine::Core::GetEngine().GetRenderer()->GetOrAddPipeline(outlinePipelineSpecs);
             
            std::shared_ptr<Rendering::Material> outlineMaterial = Rendering::Material::Create(outlineShader, outlinePipeline, false, Rendering::Opacity::Opaque);
            
            std::shared_ptr<Rendering::RenderPass> outlinePass = std::make_shared<Rendering::RenderPass>();
            outlinePass->clearColor = true;
            outlinePass->clearDepth = true;
            outlinePass->customPipeline = outlinePipeline;
            outlinePass->target = fbOutline;
            outlinePass->overridePipeline = true;
            outlinePass->customUniforms.emplace("outlineThickness", 4.0f);
            outlinePass->customUniforms.emplace("texelSize", glm::vec2(1.0 / fbOutlineMask->GetWidth(), 1.0 / fbOutlineMask->GetHeight()));
            outlinePass->customUniforms.emplace("outlineColor", glm::vec3(1.0f, 0.722f, 0.0f));
            outlinePass->customSamplers.emplace("maskTex", fbOutlineMask->GetColorAttachment());

            renderer->AddRenderPass(outlinePass, "EditorOutlinePass", {});

            Rendering::DrawCommand cmd{};
            cmd.fullscreenTri = true;
            cmd.bindCameraState = false;
            cmd.material = outlineMaterial;

            renderer->AddOrUpdateCommands({cmd}, {"EditorOutlinePass"}, false);

            for(auto model : Engine::Core::GetEngine().GetLevelManager()->GetLevelAt(0)->models)
                model.second->Update();

            renderPassesInitialized = true;
        }
        
        if(selectedActor)
            renderer->GetRenderPass("EditorOutlineMaskPass")->customUniforms.emplace("selectedObjID", selectedActor->GetID().GetAsInt());;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();


        ImGuizmo::BeginFrame();
        
        ImGui::DockSpaceOverViewport();

        assetBrowser->Draw();
        viewport->Draw();
        propertiesPanel->Draw(selectedActor);
        levelTree->Draw();
        console->Draw();

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

        viewport->ProcessInputs();
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

        ret.gpu_vendor   = Engine::Core::GetEngine().GetRenderer()->GetDeviceVendor();
        ret.gpu_renderer = Engine::Core::GetEngine().GetRenderer()->GetRendererName();
        ret.gl_version   =  Engine::Core::GetEngine().GetRenderer()->GetDriverVersion();

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
