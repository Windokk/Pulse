#pragma once

#include "engine/core/platform/iwindow.hpp"
#include "engine/rendering/renderer/renderer.hpp"
#include "engine/debugging/logger.hpp"
#include "engine/objects/actors/actor.hpp"

#include "editor/core/platform/glfw/glfw_input.hpp"
#include "editor/gui/panels/viewport/viewport_window.hpp"
#include "editor/gui/panels/properties/properties_panel.hpp"
#include "editor/gui/panels/asset_browser/asset_browser.hpp"
#include "editor/gui/panels/level_tree/level_tree.hpp"
#include "editor/gui/panels/console/console.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "imgui/backends/imgui_impl_vulkan.h"

#include <cstdint>

namespace Pulse::Editor::Core {

    enum EditorViewportBuffer{
        Final,
        Shadows,
        ObjectID,
        MatID
    };

    struct EditorSettings{

        // Viewport settings

        bool showOutlines = true;
        bool showGizmos = true;
        bool showGrid;

        /// TODO
        /// bool showShadows;
        /// bool showLights;
        /// bool showBillboards;
        /// EditorViewportBuffer currentBuffer;

        // TODO Cam settings 

    };

    class EditorMainWindow : public Engine::Core::Platform::IWindow {
    public:
        void Init(const std::string& title, const int& width, const int& height, 
                    const bool& fullscreen, const int& vsync, const uint32_t& api) override;

        void SetGLFWInputManager(GLFWInput* inputManager);

        void SetTitle(const std::string& title) override;

        void PollEvents() override;

        void SwapBuffers() override;

        bool ShouldClose() const override;

        int GetFramebufferWidth() const override;

        int GetFramebufferHeight() const override;

        void* GetNativeHandle() const override;

        void Destroy() const override;
  
        void ToggleFullscreen() override;

        void ProcessInputs() const override;

        int GetBytesPerPixel() const override;

        void SetSelectedActor(std::shared_ptr<Engine::Objects::Actor> newPtr){
            this->selectedActor = newPtr;

            if(levelTree)
                levelTree->SetSelection(newPtr);
        }

        std::shared_ptr<Engine::Objects::Actor> GetSelectedActor(){
            return selectedActor;
        }

        Engine::Core::Platform::SystemInfos GetSystemInfos() const override;

        GLFWInput* inputManager;
        
        GUI::ViewportWindow* viewport = nullptr;

        EditorSettings settings;

    private:
        bool imguiInitialized = false;
        bool renderPassesInitialized = false;

        GLFWwindow* window = nullptr;

        int windowPosX = 0;
        int windowPosY = 0;
        int windowWidth = 0;
        int windowHeight = 0;

        // Panels
        GUI::AssetBrowser* assetBrowser = nullptr;
        GUI::PropertiesPanel* propertiesPanel = nullptr;
        GUI::LevelTree* levelTree = nullptr;
        GUI::Console* console = nullptr;
        
        // User data
        std::shared_ptr<Engine::Objects::Actor> selectedActor = nullptr;
    };
}