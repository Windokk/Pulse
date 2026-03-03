#include "viewport_window.hpp"

#include "engine/core/engine.hpp"
#include "engine/rendering/opengl/opengl.hpp"
#include "engine/core/platform/iplatform.hpp"
#include "engine/ecs/components/misc/transform.reflection.hpp"

#include "editor/gui/IconsLucide.h"
#include "editor/gui/main_window.hpp"
#include "editor/gui/panels/common.hpp"
#include "editor/gui/imoguizmo.hpp"


#include <glm/gtx/string_cast.hpp>

#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>


namespace Pulse::Editor::GUI {

    ImVec2 ViewportWindow::prev_size = ImVec2(0, 0);

    void ViewportWindow::Draw()
    {
        ImGui::Begin("Viewport");
        
        ImGui::BeginChild("ToolbarArea", ImVec2(0, 26), false, ImGuiWindowFlags_NoScrollbar);
        DrawToolbar();
        ImGui::EndChild();
        
        // Get current size
        ImVec2 current_size = ImGui::GetWindowSize();

        // Rendu texture framebuffer
        uint32_t textureID = Engine::Core::GetEngine().GetRenderer()->GetViewportTextureID();

        ImVec2 avail = ImGui::GetContentRegionAvail();
        viewportSize = glm::vec2(avail.x, avail.y);

        if ((current_size.x != prev_size.x || current_size.y != prev_size.y)
            && avail.x > 0 && avail.y > 0)
        {
            Engine::Core::GetEngine().GetRenderer()->RescaleFramebuffers(avail.x, avail.y);
        }

        // Save for next frame
        prev_size = current_size;

        if (textureID != 0)
        {
            ImGui::Image(
                (void*)(intptr_t)textureID,
                ImVec2(viewportSize.x, viewportSize.y),
                ImVec2(0, 1),
                ImVec2(1, 0)
            );
        }

        viewportHovered = ImGui::IsItemHovered();
        viewportFocused = ImGui::IsWindowFocused();
        
        viewportImageMin = ImGui::GetItemRectMin();
        viewportImageMax = ImGui::GetItemRectMax();

        if(parent && parent->GetSelectedActor())
            DrawObjectGizmo();

        viewportPos = ImGui::GetWindowPos();

        ImGui::SameLine();
        DrawViewGizmo();

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

        cameraActor->transform->SetPosition(glm::vec3(10, 0, 0));

        camera = cameraActor->GetComponent<Engine::ECS::Components::Camera>();
    }

    void ViewportWindow::DrawToolbar()
    {
        ImGui::SetCursorPos(ImVec2(10, 0));

        float toolbarWidth = ImGui::GetContentRegionAvail().x;

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


        ImGui::SameLine();

        float playButtonWidth = ImGui::CalcTextSize(ICON_LC_PLAY).x + ImGui::GetStyle().FramePadding.x * 2;
        float centerX = std::max(0.0f, (toolbarWidth - playButtonWidth) / 2.0f);
        ImGui::SetCursorPosX(centerX);

        if (Engine::Core::GetEngine().IsInPlayMode())
        {
            if (ImGui::Button(ICON_LC_SQUARE)){
                Engine::Core::GetEngine().SetPlayMode(false);
            }
        }
        else
        {
            if (ImGui::Button(ICON_LC_PLAY))
            {
                auto levelManager = Engine::Core::GetEngine().GetLevelManager();
                levelManager->GetLevelAt(0)->Serialize(levelManager->GetLevelAt(0)->GetPath());
                parent->SetSelectedActor(nullptr);
                Engine::Core::GetEngine().SetPlayMode(true);
            }
        }

        
        ImGui::SameLine();
        
        float frameStatsWidth = ImGui::CalcTextSize(ICON_LC_CHART_PIE).x + ImGui::GetStyle().FramePadding.x * 2;
        float cameraWidth = ImGui::CalcTextSize(ICON_LC_CAMERA).x + ImGui::GetStyle().FramePadding.x * 2;
        float rightX = toolbarWidth - (frameStatsWidth + cameraWidth + 4); // 4px spacing
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

    void ViewportWindow::DrawViewGizmo(){
        ImVec2 gizmoPos = ImGui::GetCursorScreenPos();
        gizmoPos.x -= 120;
        gizmoPos.y += 25;

        ImOGuizmo::SetRect(gizmoPos.x, gizmoPos.y, 120.0f);

        static glm::mat4 gizmoProj = glm::perspective(glm::radians(90.0f), 4/3.0f, 0.01f, 1000.0f);

        glm::mat4 view = camera->GetView();

        int axis = ImOGuizmo::DrawGizmo(glm::value_ptr(view), glm::value_ptr(gizmoProj), 0.1f);
        if(axis != -1 && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            glm::vec3 euler;
            switch(axis)
            {
                case 0: euler = glm::vec3(0, -90, 0); break;
                case 1: euler = glm::vec3(90, 0, 0); break;
                case 2: euler = glm::vec3(0, 0, 0); break;
                case 3: euler = glm::vec3(0, 90, 0); break;
                case 4: euler = glm::vec3(-90, 0, 0); break;
                case 5: euler = glm::vec3(0, 180, 0); break;
            }
            cameraActor->transform->SetRotation(glm::quat(glm::radians(euler)));

            pitch = euler.x;
            yaw = euler.y;

            isUsingViewGizmo = true;
        }
        else if(axis != -1 && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            isUsingViewGizmo = true;
        }
        else{
            isUsingViewGizmo = false;
        }
    }

    void ViewportWindow::DrawObjectGizmo()
    {
        // Object gizmo

        auto selected = parent->GetSelectedActor();
        if (!selected)
            return;
            
        camera->UpdateMatrix();

        ImGuizmo::SetDrawlist();
        ImGuizmo::SetOrthographic(camera->IsOrthographic());

        ImVec2 imageSize = ImVec2(
            viewportImageMax.x - viewportImageMin.x,
            viewportImageMax.y - viewportImageMin.y
        );

        ImGuizmo::SetRect(
            viewportImageMin.x,
            viewportImageMin.y,
            imageSize.x,
            imageSize.y
        );
        glm::mat4 view = camera->GetView();
        glm::mat4 proj = camera->GetProjection();

        glm::mat4 transform = selected->transform->GetTransformMatrix();

        ImGuizmo::Manipulate(
            glm::value_ptr(view),
            glm::value_ptr(proj),
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

        ImGui::SliderFloat("Camera speed", &speed, 0, 100);
        ImGui::SliderFloat("Mouse sensitivity", &mouseSensitivity, 0, 10);

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
        if (!viewportFocused && !firstClick) {
            Engine::Core::GetEngine().GetInputManager()->SetCursorVisibility(true);
            firstClick = true;
        }

        if(!cameraActor || !viewportHovered || uiHovered || ImGuizmo::IsUsing() || isUsingViewGizmo)
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

            pitch += deltaY * mouseSensitivity;
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
            firstClick = true;
        }
    
        // Picking
        if(input->WasMouseReleased(Engine::Input::MouseButton::Left)){
            
            if(Engine::Core::GetEngine().GetTimeManager()->CurrentAppTime().seconds - firstClickTime > 1){
                return;
            }

            Engine::Core::EngineInstance* engine = &Engine::Core::GetEngine();

            ImVec2 mouse = ImGui::GetMousePos();

            glm::vec2 max = camera->GetSize();

            if (mouse.x >= viewportImageMin.x && mouse.x <= max.x &&
                mouse.y >= viewportImageMin.y && mouse.y <= max.y)
            {
                float localX = mouse.x - viewportImageMin.x;
                float localY = mouse.y - viewportImageMin.y;

                glm::vec3 nearPoint = camera->ScreenToWorld(localX, localY, 0.0f);
                glm::vec3 farPoint  = camera->ScreenToWorld(localX, localY, 1.0f);

                glm::vec3 dir = glm::normalize(farPoint - nearPoint);
                glm::vec3 origin = nearPoint;

                Engine::Physics::RaycastResult result = Engine::Core::GetEngine().GetPhysicsManager()->RayCast({origin, dir, 10000.0f});

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
    
    ViewportWindow::ViewportWindow() {}

    ViewportWindow::~ViewportWindow() = default;
}
