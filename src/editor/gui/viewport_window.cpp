#include "viewport_window.hpp"

#include "engine/core/engine.hpp"
#include "engine/rendering/opengl/opengl.hpp"

namespace Epoch::Editor {

    QtGLViewportWindow::QtGLViewportWindow()
        : QOpenGLWindow(QOpenGLWindow::NoPartialUpdate, nullptr)
    {
        setSurfaceType(QWindow::OpenGLSurface);
    }

    QtGLViewportWindow::~QtGLViewportWindow() = default;

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
        context()->swapBuffers(this);
    }

    QSize QtGLViewportWindow::GetFramebufferSize() const {
        return QSize(width() * devicePixelRatio(), height() * devicePixelRatio());
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
}
