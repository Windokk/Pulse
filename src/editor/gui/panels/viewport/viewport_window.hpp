#pragma once

#include "editor/core/platform/glfw/glfw_input.hpp"
#include "engine/ecs/objects/actors/actor.hpp"
#include "engine/events/event_system.hpp"

#include "editor/commands/command_stack.hpp"

#include "imgui/imgui.h"
#include "ImGuizmo.h"

namespace Pulse::Editor::Core{
    class EditorMainWindow;
}

namespace Pulse::Editor::GUI {

    class ViewportWindow
    {
        public:
            void Draw();
            void ProcessInputs();

            glm::vec2 GetViewportSize() const { return viewportSize; }
            bool IsHovered() const { return viewportHovered; }

            void SetParentWindow(Core::EditorMainWindow* parent);

        private:
            void DrawToolbar();
            void DrawGizmo();
            void ShowFrameStats();
            void ShowCamSettings();

            Core::EditorMainWindow* parent = nullptr;

            glm::vec2 viewportSize = glm::vec2(50, 50);
            ImVec2 viewportPos = ImVec2(0,0);
            bool viewportHovered = false;
            bool viewportFocused = false;

            // Camera
            std::shared_ptr<Engine::ECS::Objects::Actor> cameraActor;

            float speed = 20.0f;
            float pitch = 0.0f;
            float yaw = 0.0f;
            float mouseSensitivity = 0.1f;

            bool firstClick = true;
            double lockedMouseX = 0.0;
            double lockedMouseY = 0.0;

            bool gizmoActive = false;
            bool showFrameStats = false;
            bool showCamSettings = false;
            bool uiHovered = false;

            float firstClickTime = 0;

            // Gizmo
            ImGuizmo::OPERATION currentGizmoOp = ImGuizmo::TRANSLATE;
            ImGuizmo::MODE currentGizmoMode = ImGuizmo::LOCAL;

            std::unique_ptr<Commands::CompositeCommand> gizmoCommand;
    };
}