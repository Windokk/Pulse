#include "viewport_window.hpp"

#include "engine/core/engine.hpp"
#include "engine/rendering/opengl/opengl.hpp"
#include "engine/core/platform/iplatform.hpp"

#include "main_win.hpp"

#include "IconsLucide.h"

namespace Pulse::Editor {

    ImFont* LoadFontFromQRC(const QString& resourcePath, float fontSize, bool icons)
    {
        QFile fontFile(resourcePath);
        if (!fontFile.open(QIODevice::ReadOnly))
        {
            DEBUG_FATAL("Failed to load editor font : "+resourcePath.toStdString());
            return nullptr;
        }

        QByteArray fontData = fontFile.readAll();
        fontFile.close();

        if (fontData.isEmpty())
        {
            DEBUG_FATAL("Failed to load editor font : "+resourcePath.toStdString());
            return nullptr;
        }

        // Allocate memory ImGui can take ownership of
        void* fontMemory = malloc(fontData.size());
        memcpy(fontMemory, fontData.constData(), fontData.size());

        ImGuiIO& io = ImGui::GetIO();

        ImFontConfig cfg{};
        cfg.MergeMode = icons;
        cfg.FontDataOwnedByAtlas = true;
        cfg.PixelSnapH = true;
        cfg.GlyphOffset.y = icons ? 3.0f : 0.0f;

        const ImWchar icons_ranges[] = { ICON_MIN_LC, ICON_MAX_16_LC, 0 };

        ImFont* font;

        if(icons){
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
            DEBUG_FATAL("Failed to load editor font from ImGui memory : "+resourcePath.toStdString());

        return font;
    }

    void InitImGui(){

        ImGuiStyle& style = ImGui::GetStyle();

        style.WindowRounding = 5.0f;
        style.FrameRounding  = 3.0f;
        style.GrabRounding   = 3.0f;
        style.ScrollbarRounding = 3.0f;

        style.ItemSpacing = ImVec2(3.0f, 3.0f);

        ImVec4* colors = style.Colors;
        colors[ImGuiCol_Text]                  = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
        colors[ImGuiCol_TextDisabled]          = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
        colors[ImGuiCol_WindowBg]              = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
        colors[ImGuiCol_ChildBg]               = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
        colors[ImGuiCol_PopupBg]               = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
        colors[ImGuiCol_Border]                = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
        colors[ImGuiCol_BorderShadow]          = ImVec4(0, 0, 0, 0);
        colors[ImGuiCol_FrameBg]               = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
        colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
        colors[ImGuiCol_FrameBgActive]         = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);
        colors[ImGuiCol_TitleBg]               = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
        colors[ImGuiCol_TitleBgActive]         = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
        colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
        colors[ImGuiCol_MenuBarBg]             = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
        colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
        colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
        colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
        colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
        colors[ImGuiCol_CheckMark]             = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
        colors[ImGuiCol_SliderGrab]            = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
        colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
        colors[ImGuiCol_Button]                = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
        colors[ImGuiCol_ButtonHovered]         = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);
        colors[ImGuiCol_ButtonActive]          = ImVec4(0.45f, 0.45f, 0.45f, 1.0f);
        colors[ImGuiCol_Header]                = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
        colors[ImGuiCol_HeaderHovered]         = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);
        colors[ImGuiCol_HeaderActive]          = ImVec4(0.45f, 0.45f, 0.45f, 1.0f);
        colors[ImGuiCol_Separator]             = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
        colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
        colors[ImGuiCol_SeparatorActive]       = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
        colors[ImGuiCol_ResizeGrip]            = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
        colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
        colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
        colors[ImGuiCol_PlotLines]             = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
        colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
        colors[ImGuiCol_PlotHistogram]         = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
        colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
        colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);

        LoadFontFromQRC(":pulse/default/fonts/OpenSans-Regular.ttf", 15, false);
        LoadFontFromQRC(":pulse/default/fonts/lucide.ttf", 15, true);
    }

    QtGLViewportWindow::QtGLViewportWindow()
        : QOpenGLWindow(QOpenGLWindow::NoPartialUpdate, nullptr)
    {
        setSurfaceType(QWindow::OpenGLSurface);

        Engine::Core::GetEngine().GetEventDispatcher()->subscribeGlobal<Engine::Events::LevelStructureChangedEvent>([this](const Engine::Events::LevelStructureChangedEvent& event) {
            this->OnLevelStructureChanged(event);
        });
    }

    void QtGLViewportWindow::InitGL(bool vsync) {
        QSurfaceFormat fmt;
        fmt.setVersion(4, 3);
        fmt.setProfile(QSurfaceFormat::CoreProfile);
        fmt.setDepthBufferSize(24);
        fmt.setStencilBufferSize(8);
        fmt.setSwapInterval(vsync ? 1 : 0);
        fmt.setSamples(4);
        setFormat(fmt);
    }

    void QtGLViewportWindow::initializeGL() {
        OpenGL* gl = new OpenGL();
        gl->InitFromQt();
        Engine::Core::GetEngine().SetGL(gl);
        initialized = true;

        
        initializeOpenGLFunctions();
        QtImGui::Initialize(this);

        InitImGui();
    }

    void QtGLViewportWindow::resizeGL(int w, int h) {
        if (!initialized || !Engine::Core::GetEngine().GetRenderer()->initialized || Engine::Core::GetEngine().GetWindow()->ShouldClose()) return;

        Engine::Core::GetEngine().GetRenderer()->RescaleFramebuffers(w * devicePixelRatio(), h * devicePixelRatio());
    }

    void QtGLViewportWindow::paintGL() {
    }

    void QtGLViewportWindow::MakeCurrent() {
        this->makeCurrent();
    }

    void QtGLViewportWindow::SwapBuffers() {
        
        if(parent && parent->ShouldClose())
            return;

        QtImGui::NewFrame();
        
        ImGuizmo::BeginFrame();

        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();

        ImGui::SetNextWindowPos(ImVec2(width() * devicePixelRatio() - 115.0f, 0.0f));
        ImGui::Begin("##GizmoSelection", nullptr, 
            ImGuiWindowFlags_NoBackground  |
            ImGuiWindowFlags_NoTitleBar    | 
            ImGuiWindowFlags_NoResize      | 
            ImGuiWindowFlags_NoCollapse    | 
            ImGuiWindowFlags_NoMove        | 
            ImGuiWindowFlags_AlwaysAutoResize);
        auto DrawGizmoButton = [&](const char* icon, ImGuizmo::OPERATION op)
        {
            bool selected = (currentGizmoOp == op);
            if (selected)
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            else
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_Button));

            if (ImGui::Button(icon))
                currentGizmoOp = op;

            ImGui::PopStyleColor();
        };

        // Draw buttons
        DrawGizmoButton(ICON_LC_MOVE, ImGuizmo::TRANSLATE);
        ImGui::SameLine();
        DrawGizmoButton(ICON_LC_REFRESH_CW, ImGuizmo::ROTATE);
        ImGui::SameLine();
        DrawGizmoButton(ICON_LC_SCALING, ImGuizmo::SCALE);
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(width() * devicePixelRatio() - 35.0f, 0.0f));
        ImGui::Begin("##ToggleStats", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);
        if(showFrameStats)
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        else
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_Button));
        if(ImGui::Button(ICON_LC_CHART_PIE))
            showFrameStats = !showFrameStats;
        ImGui::PopStyleColor();
        ImGui::End();

        if(showFrameStats)
        {
            ImGui::SetNextWindowPos(ImVec2(width() * devicePixelRatio() - 200.0f, 34.0f));
            ImGui::Begin("##Stats", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Text("Statistics");
            ImGui::Separator();

            // Display frame metrics
            Engine::Debugging::MinimalStatistics stats = Engine::Core::GetEngine().GetProfiler()->GetStats();
 
            ImGui::Text("Sounds: %d", stats.sounds);

            // Rendering stats
            ImGui::Text("Frame Time: %.2f ms (%.1f FPS)", stats.frameTimeMs, stats.fps);
            ImGui::Text("Draw Calls: %d", stats.drawCalls);
            ImGui::Text("Triangles: %d", stats.triangles);
            ImGui::Text("Vertices: %d", stats.vertices);
            ImGui::Text("GPU Memory: %.1f MB", stats.gpuMemoryMB);

            // Level stats
            ImGui::Text("Actors: %d", stats.actors);
            ImGui::Text("Lights: %d", stats.lights);

            ImGui::End();
        }
        

        std::shared_ptr<Engine::ECS::Objects::Actor> selected = parent->GetSelectedActor();

        if(selected != nullptr){

            ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

            const float* cameraView = glm::value_ptr(Engine::Core::GetEngine().GetCameraManager()->GetActiveCamera()->GetView());
            const float* cameraProjection = glm::value_ptr(Engine::Core::GetEngine().GetCameraManager()->GetActiveCamera()->GetProjection());

            glm::mat4 tr = selected->transform->GetTransformMatrix();

            ImGuizmo::Manipulate(cameraView, cameraProjection, currentGizmoOp, currentGizmoMode, glm::value_ptr(tr), NULL, NULL);

            selected->transform->SetFromTransformMatrix(tr);
        }

        uiHovered = ImGui::IsAnyItemActive();

        ImGui::Render();
        QtImGui::Render();

        context()->swapBuffers(this);
    }

    QSize QtGLViewportWindow::GetFramebufferSize() const {
        return QSize(width() * devicePixelRatio(), height() * devicePixelRatio());
    }

    void QtGLViewportWindow::SetParentWindow(EditorMainWindow *parent)
    {
        this->parent = parent;
    }

    bool QtGLViewportWindow::event(QEvent* e) {
        switch (e->type()) {
            case QEvent::KeyPress:
            case QEvent::KeyRelease: {
                auto* key = static_cast<QKeyEvent*>(e);
                if (inputManager)
                    inputManager->KeyCallback(key->key(), key->type() == QEvent::KeyPress ? 1 : 0);
                break;
            }
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonRelease: {
                auto* mouse = static_cast<QMouseEvent*>(e);
                if (inputManager)
                    inputManager->MouseCallback(mouse->button(), mouse->type() == QEvent::MouseButtonPress ? 1 : 0);
                break;
            }
        }
        return QOpenGLWindow::event(e);
    }

    void QtGLViewportWindow::SetQTInputManager(Core::Platform::QTInput* input) {
        inputManager = input;
    }

    void QtGLViewportWindow::OnLevelStructureChanged(Engine::Events::LevelStructureChangedEvent event){

        switch(event.changeType){
            case Engine::Events::LOADED:{
                cameraActor = Engine::ECS::Objects::Object::Create<Engine::ECS::Objects::Actor>("[EDITOR] Camera");
                cameraActor->AddComponent<Engine::ECS::Components::Camera>();
                int width = Engine::Core::GetEngine().GetWindow()->GetFramebufferWidth();
                int height = Engine::Core::GetEngine().GetWindow()->GetFramebufferHeight();
                cameraActor->GetComponent<Engine::ECS::Components::Camera>()->Init(width, height, near, far);
                Engine::Core::GetEngine().GetCameraManager()->AddCamera(cameraActor->GetID(), cameraActor->GetComponent<Engine::ECS::Components::Camera>());
                Engine::Core::GetEngine().GetCameraManager()->SetActiveCamera(cameraActor->GetID());
                break;
            }
            default:
                break;
        }
    }

    void QtGLViewportWindow::ProcessInputs()
    {
        if(!cameraActor)
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

        if(ImGuizmo::IsUsing() || uiHovered)
            return;
        
        if (input->IsMouseDown(Engine::Input::MouseButton::Left))
        {
            input->SetCursorVisibility(false);

            double mouseX, mouseY;
            input->GetCursorPos(&mouseX, &mouseY);

            if (firstClick)
            {
                lockedMouseX = mouseX;
                lockedMouseY = mouseY;
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
            firstClick = true;
            input->SetCursorVisibility(true);
        }
    }
}
