#pragma once

#include <QOpenGLWindow>
#include "editor/core/platform/qt/qt_input.hpp"

namespace Epoch::Editor {
    class QtGLViewportWindow : public QOpenGLWindow {
    public:
        QtGLViewportWindow();
        ~QtGLViewportWindow() override;

        void InitGL(bool vsync);
        void MakeCurrent();
        void SwapBuffers();
        QSize GetFramebufferSize() const;

        void SetQTInputManager(Core::Platform::QTInput* input);

        bool initialized = false;

    protected:
        void initializeGL() override;
        void resizeGL(int w, int h) override;
        void paintGL() override;
        bool event(QEvent* e) override;

    private:
        Core::Platform::QTInput* inputManager = nullptr;
    };
}