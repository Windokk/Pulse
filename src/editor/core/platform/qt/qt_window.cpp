#include "qt_window.hpp"
#include <QResizeEvent>
#include <QCloseEvent>

#include "engine/core/engine.hpp"

#include "engine/rendering/opengl/opengl.hpp"

namespace Epoch::Engine::Core::Platform {

    QTWindow::QTWindow() {
        setSurfaceType(QWindow::OpenGLSurface);
    }

    QTWindow::~QTWindow() {
        if (context) {
            context->doneCurrent();
            delete context;
        }
    }

    bool QTWindow::event(QEvent* e) {
        switch (e->type()) {
            case QEvent::KeyPress:
            case QEvent::KeyRelease: {
                QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
                inputManager->KeyCallback(keyEvent->key(), keyEvent->type() == QEvent::KeyPress ? 1 : 0);
                break;
            }
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonRelease: {
                QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(e);
                inputManager->MouseCallback(mouseEvent->button(), mouseEvent->type() == QEvent::MouseButtonPress ? 1 : 0);
                break;
            }
            // add other event types if needed
        }
        return QWindow::event(e);
    }

    static void* QtGetProcAddress(const char* name) {
        return (void*)QOpenGLContext::currentContext()->getProcAddress(name);
    }

    void QTWindow::Init(const std::string& title, const int& width, const int& height,
                        const bool& fullscreen, const int& vsync) {

        QSurfaceFormat format;
        format.setDepthBufferSize(24);
        format.setStencilBufferSize(8);
        format.setMajorVersion(4);
        format.setMinorVersion(3);
        format.setSamples(4);
        format.setSwapInterval(vsync);
        format.setProfile(QSurfaceFormat::CoreProfile);
        setFormat(format);

        resize(width, height);
        setTitle(QString::fromStdString(title));

        show();

        if (fullscreen) {
            windowedGeometry = geometry();
            setWindowState(Qt::WindowFullScreen);
            isFullscreen = true;
        }

        context = new QOpenGLContext(this);
        context->setFormat(format);
        context->create();
        context->makeCurrent(this);

        GetGL().InitFromQt();
    }

    void QTWindow::PollEvents() {
        QCoreApplication::processEvents();
    }

    void QTWindow::SwapBuffers() {
        context->swapBuffers(this);
    }

    bool QTWindow::ShouldClose() const {
        return shouldClose;
    }

    int QTWindow::GetFramebufferWidth() const {
        return width() * devicePixelRatio();
    }

    int QTWindow::GetFramebufferHeight() const {
        return height() * devicePixelRatio();
    }

    void* QTWindow::GetNativeHandle() const {
        return reinterpret_cast<void*>(winId());
    }

    void QTWindow::Destroy() const {
        const_cast<QTWindow*>(this)->close();
    }

    void QTWindow::ToggleFullscreen() {
        if (isFullscreen) {
            setWindowState(Qt::WindowNoState);
            setGeometry(windowedGeometry);
            isFullscreen = false;
            Rendering::Renderer::GetInstance().RescaleFramebuffers(windowedGeometry.width(), windowedGeometry.height());
        } else {
            windowedGeometry = geometry();
            setWindowState(Qt::WindowFullScreen);
            isFullscreen = true;
            Rendering::Renderer::GetInstance().RescaleFramebuffers(windowedGeometry.width(), windowedGeometry.height());
        }

        Rendering::Renderer::GetInstance().RescaleFramebuffers(GetFramebufferWidth(), GetFramebufferHeight());
    }

    SystemInfos QTWindow::GetSystemInfos() const {
        SystemInfos ret{};

        // GL Strings
        const GLubyte* vendor   = GetGL().GetString(GL_VENDOR);
        const GLubyte* renderer = GetGL().GetString(GL_RENDERER);
        const GLubyte* version  = GetGL().GetString(GL_VERSION);

        ret.gpu_vendor   = reinterpret_cast<const char*>(vendor);
        ret.gpu_renderer = reinterpret_cast<const char*>(renderer);
        ret.gl_version   = reinterpret_cast<const char*>(version);

        // Qt version
        ret.windowHostVersion = QT_VERSION_STR;
        ret.windowHost = QT;

        // Monitor info
        auto screens = QGuiApplication::screens();
        ret.connectedMonitorsCount = static_cast<int>(screens.size());

        for (auto* screen : screens) {
            QSize res = screen->size();
            int refreshRate = static_cast<int>(screen->refreshRate());
            ret.monitors.push_back({ res.width(), res.height(), refreshRate });
        }

        return ret;
    }

    void QTWindow::resizeEvent(QResizeEvent* event) {
        QWindow::resizeEvent(event);
        Rendering::Renderer::GetInstance().RescaleFramebuffers(GetFramebufferWidth(), GetFramebufferHeight());
    }

    void QTWindow::closeEvent(QCloseEvent* event) {
        shouldClose = true;
        event->accept();
    }
}