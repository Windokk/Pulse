#pragma once

#include <QOpenGLWindow>
#include <QOpenGLExtraFunctions>

#include <imgui/imgui.h>
#include <QtImGui.h>
#include <ImGuizmo.h>

#include "editor/core/platform/qt/qt_input.hpp"
#include "engine/ecs/objects/actors/actor.hpp"
#include "engine/events/event_system.hpp"

namespace Pulse::Editor {

    class EditorMainWindow;

    class QtGLViewportWindow : public QOpenGLWindow, private QOpenGLExtraFunctions {
    public:
        QtGLViewportWindow();

        void InitGL(bool vsync);
        void MakeCurrent();
        void SwapBuffers();
        QSize GetFramebufferSize() const;

        void SetParentWindow(EditorMainWindow* parent);

        void SetQTInputManager(Core::Platform::QTInput* input);

        void OnLevelStructureChanged(Engine::Events::LevelStructureChangedEvent event);

        void ProcessInputs();

        bool initialized = false;

    protected:
        void initializeGL() override;
        void resizeGL(int w, int h) override;
        void paintGL() override;
        bool event(QEvent* e) override;

    private:
        Core::Platform::QTInput* inputManager = nullptr;
        EditorMainWindow* parent = nullptr;

        //Gizmo
        ImGuizmo::OPERATION currentGizmoOp = ImGuizmo::TRANSLATE;
        ImGuizmo::MODE currentGizmoMode = ImGuizmo::WORLD;

        //Frame Stats
        bool showFrameStats = false;

        //Camera
        std::shared_ptr<Engine::ECS::Objects::Actor> cameraActor = nullptr;
        float speed = 20.0f;
        double lockedMouseX, lockedMouseY = 0;
        bool firstClick = true;
        float pitch = 0.0f;
        float yaw = 0.0f;
        float mouseSensitivity = 0.1f;
        float near = 0.1f;
        float far = 100.0f;

        bool uiHovered = false;
    };
}