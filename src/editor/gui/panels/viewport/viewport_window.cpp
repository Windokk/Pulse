#include "viewport_window.hpp"

#include "engine/core/engine.hpp"
#include "engine/rendering/opengl/opengl.hpp"
#include "engine/core/platform/iplatform.hpp"
#include "engine/ecs/components/misc/transform.reflection.hpp"

#include "editor/gui/IconsLucide.h"
#include "editor/gui/main_window.hpp"

#include <glm/gtx/string_cast.hpp>

#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>

namespace Pulse::Editor::GUI {

    void ViewportWindow::Draw()
    {   
        // Store previous size
        static ImVec2 prev_size = ImGui::GetWindowSize();

        ImGui::Begin("Viewport");

        // Get current size
        ImVec2 current_size = ImGui::GetWindowSize();

        viewportHovered = ImGui::IsWindowHovered();
        viewportFocused = ImGui::IsWindowFocused();

        // Rendu texture framebuffer
        uint32_t textureID = Engine::Core::GetEngine().GetRenderer()->GetViewportTextureID();

        // Check if it changed
        if (current_size.x != prev_size.x || current_size.y != prev_size.y) {
            // Window was resized
            Engine::Core::GetEngine().GetRenderer()->RescaleFramebuffers(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
            viewportSize = glm::vec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
        }

        // Save for next frame
        prev_size = current_size;

        ImGui::Image(
            (void*)(intptr_t)textureID,
            ImVec2(viewportSize.x, viewportSize.y),
            ImVec2(0, 1),
            ImVec2(1, 0)
        );

        DrawToolbar();
        DrawGizmo();

        viewportPos = ImGui::GetWindowPos();

        uiHovered = ImGui::IsAnyItemHovered();

        ImGui::End();
    }

    void ViewportWindow::SetParentWindow(Core::EditorMainWindow *parent)
    {
        this->parent = parent;

        cameraActor = Engine::Core::Object::Create<Engine::ECS::Objects::Actor>("[EDITOR] Camera");
        cameraActor->AddComponent<Engine::ECS::Components::Camera>();
        int width = Engine::Core::GetEngine().GetWindow()->GetFramebufferWidth();
        int height = Engine::Core::GetEngine().GetWindow()->GetFramebufferHeight();
        cameraActor->GetComponent<Engine::ECS::Components::Camera>()->Init(width, height, 0.1f, 100.0f, 60, false, 10);
        Engine::Core::GetEngine().GetCameraManager()->AddCamera(cameraActor->GetID(), cameraActor->GetComponent<Engine::ECS::Components::Camera>());
        Engine::Core::GetEngine().GetCameraManager()->SetActiveCamera(cameraActor->GetID());
    }

    void ViewportWindow::DrawToolbar()
    {
        ImGui::SetCursorPos(ImVec2(10, 32));

        float toolbarWidth = ImGui::GetContentRegionAvail().x;

        // ------------------ LEFT ------------------
        // Gizmo Space
        const char* items[2] = { ICON_LC_LOCATE " Local", ICON_LC_EARTH " World" };
        static int item_selected_idx = 0;
        const char* combo_preview_value = items[item_selected_idx];
        ImGui::PushItemWidth(100);
        if (ImGui::BeginCombo("##GizmoSpace", combo_preview_value, 0))
        {
            for (int n = 0; n < IM_ARRAYSIZE(items); n++)
            {
                const bool is_selected = (item_selected_idx == n);
                if (ImGui::Selectable(items[n], is_selected))
                    item_selected_idx = n;
                if (is_selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        currentGizmoMode = (item_selected_idx == 0) ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

        ImGui::SameLine();

        // Gizmo Operations (packed to the left)
        auto DrawGizmoButton = [&](const char* icon, ImGuizmo::OPERATION op)
        {
            bool selected = (currentGizmoOp == op);
            if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            if (ImGui::Button(icon)) currentGizmoOp = op;
            if (selected) ImGui::PopStyleColor();
        };

        DrawGizmoButton(ICON_LC_MOVE, ImGuizmo::TRANSLATE);
        ImGui::SameLine();
        DrawGizmoButton(ICON_LC_REFRESH_CW, ImGuizmo::ROTATE);
        ImGui::SameLine();
        DrawGizmoButton(ICON_LC_SCALING, ImGuizmo::SCALE);

        // ------------------ CENTER ------------------

        ImGui::SameLine();

        float playButtonWidth = ImGui::CalcTextSize(ICON_LC_PLAY).x + ImGui::GetStyle().FramePadding.x * 2;
        float centerX = (toolbarWidth - playButtonWidth) / 2.0f;
        ImGui::SetCursorPosX(centerX);

        if (Engine::Core::GetEngine().IsInPlayMode())
        {
            if (ImGui::Button(ICON_LC_SQUARE))
                Engine::Core::GetEngine().SetPlayMode(false);
        }
        else
        {
            if (ImGui::Button(ICON_LC_PLAY))
            {
                auto levelManager = Engine::Core::GetEngine().GetLevelManager();
                levelManager->GetLevelAt(0)->Serialize(levelManager->GetLevelAt(0)->GetPath());
                Engine::Core::GetEngine().SetPlayMode(true);
            }
        }

        // ------------------ RIGHT ------------------
        
        ImGui::SameLine();
        
        float frameStatsWidth = ImGui::CalcTextSize(ICON_LC_CHART_PIE).x + ImGui::GetStyle().FramePadding.x * 2;
        float cameraWidth = ImGui::CalcTextSize(ICON_LC_CAMERA).x + ImGui::GetStyle().FramePadding.x * 2;
        float rightX = toolbarWidth - (frameStatsWidth + cameraWidth + 10); // 10px spacing
        ImGui::SetCursorPosX(rightX);

        // Frame Stats Button
        {
            if(showFrameStats)
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            else
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_Button));
            if(ImGui::Button(ICON_LC_CHART_PIE))
                showFrameStats = !showFrameStats;
            ImGui::PopStyleColor();

            if(showFrameStats)
            {
                ShowFrameStats();
            }
        }

        ImGui::SameLine();

        // Camera settings
        {
            if(showCamSettings)
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            else
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_Button));
            if(ImGui::Button(ICON_LC_CAMERA))
                showCamSettings = !showCamSettings;
            ImGui::PopStyleColor();

            if(showCamSettings)
            {
                ShowCamSettings();
            }
        }
    }

    void ViewportWindow::DrawGizmo()
    {
        auto selected = parent->GetSelectedActor();
        if (!selected)
            return;

        ImGuizmo::BeginFrame();

        auto cam = Engine::Core::GetEngine().GetCameraManager()->GetActiveCamera();

        ImGuizmo::SetOrthographic(cam->IsOrthographic());
        ImGuizmo::SetDrawlist();

        ImVec2 viewportMin = ImGui::GetWindowContentRegionMin();
        ImVec2 viewportMax = ImGui::GetWindowContentRegionMax();
        ImVec2 windowPos   = ImGui::GetWindowPos();

        ImVec2 rectMin = {
            windowPos.x + viewportMin.x,
            windowPos.y + viewportMin.y
        };

        ImVec2 rectSize = {
            viewportMax.x - viewportMin.x,
            viewportMax.y - viewportMin.y
        };

        ImGuizmo::SetRect(rectMin.x, rectMin.y, rectSize.x, rectSize.y);

        const float* view = glm::value_ptr(cam->GetView());
        const float* proj = glm::value_ptr(cam->GetProjection());

        glm::mat4 transform = selected->transform->GetTransformMatrix();

        ImGuizmo::Manipulate(
            view,
            proj,
            currentGizmoOp,
            currentGizmoMode,
            glm::value_ptr(transform)
        );

        if (ImGuizmo::IsUsing() && !gizmoActive) {
            gizmoActive = true;

            auto& stack = Commands::CommandStack::Get();
            stack.Begin("Move Object");

            for (FieldInfo* field :
                selected->transform->GetDescriptor()->fields)
            {
                stack.Add(
                    std::make_unique<Commands::ModifyFieldCommand>(
                        selected->transform.get(), field
                    )
                );
            }
        }

        if (ImGuizmo::IsUsing()) {
            selected->transform->SetFromTransformMatrix(transform);
        }

        if (!ImGuizmo::IsUsing() && gizmoActive) {
            gizmoActive = false;
            Commands::CommandStack::Get().End();
        }
    }

    void TextEllipsis(const char* text)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return;

        ImGuiContext& g = *ImGui::GetCurrentContext();
        const ImGuiStyle& style = g.Style;

        ImVec2 pos = window->DC.CursorPos;
        float width = ImGui::GetContentRegionAvail().x;
        float height = ImGui::GetTextLineHeight();

        ImGui::ItemSize(ImVec2(width, height));
        ImGui::ItemAdd(
            ImRect(
                pos,
                ImVec2(pos.x + width, pos.y + height)
            ),
            0
        );

        ImVec2 pos_max(pos.x + width, pos.y + height);

        ImGui::RenderTextEllipsis(
            window->DrawList,
            pos,
            pos_max,
            pos.x + width, // ellipsis_max_x
            text,
            nullptr,
            nullptr
        );
    }

    void ViewportWindow::ShowFrameStats(){

        ImGui::Begin("##Stats", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Statistics");
        ImGui::Separator();

        // Display frame metrics
        Engine::Debugging::MinimalStatistics stats = Engine::Core::GetEngine().GetProfiler()->GetStats();
        Engine::Core::Platform::SystemInfos system = Engine::Core::GetEngine().GetWindow()->GetSystemInfos();

        ImGui::Text("Sounds: %d", stats.sounds);

        ImGui::Separator();

        // Rendering stats
        ImGui::Text("Frame Time: %.2f ms (%.1f FPS)", stats.frameTimeMs, stats.fps);
        ImGui::Text("Draw Commands: %d", stats.cmds);
        ImGui::Text("Triangles: %d", stats.triangles);
        ImGui::Text("Vertices: %d", stats.vertices);
        ImGui::Text("GPU Memory: %.1f MB", stats.gpuMemoryMB);

        ImGui::Separator();

        // Level stats
        ImGui::Text("Actors: %d", stats.actors);
        ImGui::Text("Lights: %d", stats.lights);

        ImGui::Separator();

        std::string text = "Renderer : " + system.gpu_renderer;
        TextEllipsis(text.c_str());
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", system.gpu_renderer.c_str());
        }

        ImGui::End();
    }

    void ViewportWindow::ShowCamSettings(){
        
        ImGui::Begin("##Cam", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Scene Camera");
        ImGui::Separator();

        std::shared_ptr<Engine::ECS::Components::Camera> cam = cameraActor->GetComponent<Engine::ECS::Components::Camera>();

        // Rendering stats
        ImGui::Text("Camera width : %f px", cam->GetSize().x);
        ImGui::Text("Camera height : %f px", cam->GetSize().y);

        ImGui::Separator();

        ImGui::SliderFloat("Camera speed", &speed, 0, 1000);
        ImGui::SliderFloat("Mouse sensitivity", &mouseSensitivity, 0, 1000);

        ImGui::Separator();

        ImGui::SliderFloat("Near plane", &cam->nearPlane, 0.01f, 1000);
        ImGui::SliderFloat("Far plane", &cam->farPlane, cam->nearPlane + 1, 1000);

        ImGui::Checkbox("Ortho ?", &cam->orthographic);

        if(cam->IsOrthographic()){
            ImGui::SliderFloat("Ortho size", cam->GetOrthoSize(), 1.0f, 100.0f);
        }
        else{
            ImGui::SliderFloat("FOV", cam->GetFOV(), 1.0f, 150.0f);
        }

        ImGui::End();
    }

    void ViewportWindow::ProcessInputs()
    {
        if(!cameraActor || !viewportHovered || uiHovered || ImGuizmo::IsUsing())
            return;

        Engine::Core::Platform::IInput* input = Engine::Core::GetEngine().GetInputManager();

        Engine::Time::TimeManager* time = Engine::Core::GetEngine().GetTimeManager();

        if(input->IsKeyDown(Engine::Input::Key::W)){
            cameraActor->transform->Translate(cameraActor->transform->GetForward() * speed * time->GetDeltaTime());
        }
        if(input->IsKeyDown(Engine::Input::Key::A)){
            cameraActor->transform->Translate(glm::normalize(glm::cross(cameraActor->transform->GetForward(), cameraActor->transform->GetUp())) * -speed * time->GetDeltaTime());
        }
        if(input->IsKeyDown(Engine::Input::Key::S)){
            cameraActor->transform->Translate(cameraActor->transform->GetForward() * -speed * time->GetDeltaTime());
        }
        if(input->IsKeyDown(Engine::Input::Key::D)){
            cameraActor->transform->Translate(glm::normalize(glm::cross(cameraActor->transform->GetForward(), cameraActor->transform->GetUp())) * speed * time->GetDeltaTime());
        }
        
        if (input->IsMouseDown(Engine::Input::MouseButton::Left))
        {
            input->SetCursorVisibility(false);
            double mouseX, mouseY;
            input->GetCursorPos(&mouseX, &mouseY);

            if (firstClick)
            {
                lockedMouseX = mouseX;
                lockedMouseY = mouseY;
                firstClickTime = Engine::Core::GetEngine().GetTimeManager()->CurrentAppTime().seconds;
                firstClick = false;
            }

            double deltaX = mouseX - lockedMouseX;
            double deltaY = lockedMouseY - mouseY; // reversed Y

            pitch -= deltaY * mouseSensitivity;
            yaw   -= deltaX * mouseSensitivity;

            // Clamp pitch to avoid flipping
            pitch = glm::clamp(pitch, -89.0f, 89.0f);

            // Build quaternion from yaw * pitch
            glm::quat qPitch = glm::angleAxis(glm::radians(pitch), glm::vec3(1, 0, 0));
            glm::quat qYaw   = glm::angleAxis(glm::radians(yaw),   glm::vec3(0, 1, 0));
            glm::quat rotation = qYaw * qPitch;
            
            cameraActor->transform->SetRotation(rotation);
    
            // Reset cursor back to locked position every frame
            input->SetCursorPos(lockedMouseX, lockedMouseY);
        }
        if(input->IsMouseUp(Engine::Input::MouseButton::Left))
        {
            input->SetCursorVisibility(true);
            firstClick = true;
        }
    
        // Picking
        if(input->WasMouseReleased(Engine::Input::MouseButton::Left)){
            
            if(Engine::Core::GetEngine().GetTimeManager()->CurrentAppTime().seconds - firstClickTime > 1){
                return;
            }

            ImGui::SetMouseCursor(ImGuiMouseCursor_None);

            Engine::Core::EngineInstance* engine = &Engine::Core::GetEngine();

            glm::dvec2 screenPos;
            input->GetCursorPos(&screenPos.x, &screenPos.y);

            ImVec2 mouse = ImGui::GetMousePos();
            ImVec2 winPos = viewportPos;

            float localX = mouse.x - winPos.x;
            float localY = mouse.y - winPos.y;

            glm::vec3 origin = cameraActor->transform->GetPosition();

            glm::vec3 dir = engine->GetCameraManager()->GetActiveCamera()->GetWorldPointFromScreenPoint(glm::vec2(localX, localY));
            dir = glm::normalize(dir);
            dir *= engine->GetCameraManager()->GetActiveCamera()->farPlane;

            Engine::Physics::RaycastResult result = Engine::Core::GetEngine().GetPhysicsManager()->RayCast({origin, dir, engine->GetCameraManager()->GetActiveCamera()->farPlane});

            if (result.hit)
            {
                parent->SetSelectedActor(result.hitBody->parent);
            }
            else{
                parent->SetSelectedActor(nullptr);
            }
        }

    }
}
