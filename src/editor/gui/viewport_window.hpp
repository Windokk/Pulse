#pragma once

#include <QOpenGLWindow>
#include <QOpenGLExtraFunctions>

#include "editor/core/platform/qt/qt_input.hpp"

namespace Pulse::Editor {

    class EditorMainWindow;

    class QtGLViewportWindow : public QOpenGLWindow, private QOpenGLExtraFunctions {
    public:
        QtGLViewportWindow();
        ~QtGLViewportWindow() override;

        void InitGL(bool vsync);
        void MakeCurrent();
        void SwapBuffers();
        QSize GetFramebufferSize() const;

        void SetParentWindow(EditorMainWindow* parent);

        void SetQTInputManager(Core::Platform::QTInput* input);

        bool initialized = false;

    protected:
        void initializeGL() override;
        void resizeGL(int w, int h) override;
        void paintGL() override;
        bool event(QEvent* e) override;

    private:
        Core::Platform::QTInput* inputManager = nullptr;
        EditorMainWindow* parent = nullptr;
    };
}