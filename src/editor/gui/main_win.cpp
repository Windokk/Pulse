#include "main_win.hpp"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QWidget>
#include <QToolButton>

#include "engine/core/engine.hpp"

#include <DockAreaWidget.h>
#include <DockAreaTabBar.h>

#include "editor/gui/level_tree.hpp"


namespace Pulse::Editor
{



    void EditorMainWindow::Init(const std::string& title, const int& width, const int& height,
                        const bool& fullscreen, const int& vsync) {

        if (fullscreen) {
            setWindowState(Qt::WindowFullScreen);
        }

        this->vsync = vsync;

        setWindowTitle(QString::fromStdString(title));

        setAttribute(Qt::WA_DeleteOnClose);
        resize(width, height);

        InitGui();
    }

    void EditorMainWindow::InitGui(){
        
        dockManager = std::make_unique<ads::CDockManager>(this);

        dockManager->setStyleSheet("");

        QFile styleSheetFile(":/pulse/default/stylesheets/default_dock.qss");
	    styleSheetFile.open(QIODevice::ReadOnly);
	    QTextStream styleSheetStream(&styleSheetFile);
	    QString result;
	    result = styleSheetStream.readAll();
	    styleSheetFile.close();
	    dockManager->setStyleSheet(result);

        glViewportWindow = new Editor::QtGLViewportWindow();
        glViewportWindow->SetParentWindow(this);
        glViewportWindow->InitGL(vsync);

        auto* container = QWidget::createWindowContainer(glViewportWindow);
        container->setFocusPolicy(Qt::StrongFocus);

        auto* viewportDock = dockManager->createDockWidget("Viewport");
        viewportDock->setWidget(container);
        viewportDock->setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetFloatable, false);
        
        auto* treeWidget = new LevelTree();
        treeWidget->SetParentWindow(this);

        auto* levelTreeDock = dockManager->createDockWidget("Level Tree");
        levelTreeDock->setWidget(treeWidget);
        levelTreeDock->setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetFloatable, false);

        dockManager->addDockWidget(ads::CenterDockWidgetArea, viewportDock);
        dockManager->addDockWidget(ads::LeftDockWidgetArea, levelTreeDock);

        show();
    }

    void EditorMainWindow::PollEvents()
    {
        QCoreApplication::processEvents();
    }

    void EditorMainWindow::SwapBuffers()
    {
        glViewportWindow->SwapBuffers();
    }

    bool EditorMainWindow::ShouldClose() const {
        return shouldClose;
    }

    int EditorMainWindow::GetFramebufferWidth() const
    {
        return glViewportWindow->width() * devicePixelRatio();
    }

    int EditorMainWindow::GetFramebufferHeight() const
    {
        return glViewportWindow->height() * devicePixelRatio();
    }

    void *EditorMainWindow::GetNativeHandle() const
    {
        return reinterpret_cast<void*>(winId());
    }

    void EditorMainWindow::Destroy() const
    {
        const_cast<EditorMainWindow*>(this)->close();
    }

    void EditorMainWindow::ToggleFullscreen()
    {
        if (isFullScreen()) {
            showNormal();
        } else {
            showFullScreen();
        }
    }

    Engine::Core::Platform::SystemInfos EditorMainWindow::GetSystemInfos() const
    {
        if(!glViewportWindow->initialized)
            return {};

        Engine::Core::Platform::SystemInfos ret{};

        // GL Strings
        const GLubyte* vendor   = Engine::Core::GetEngine().GetGL()->GetString(GL_VENDOR);
        const GLubyte* renderer = Engine::Core::GetEngine().GetGL()->GetString(GL_RENDERER);
        const GLubyte* version  = Engine::Core::GetEngine().GetGL()->GetString(GL_VERSION);

        ret.gpu_vendor   = vendor   ? reinterpret_cast<const char*>(vendor)   : "Unknown";
        ret.gpu_renderer = renderer ? reinterpret_cast<const char*>(renderer) : "Unknown";
        ret.gl_version   = version  ? reinterpret_cast<const char*>(version)  : "Unknown";

        // Qt version
        ret.windowHostVersion = QT_VERSION_STR;
        ret.windowHost = Engine::Core::Platform::QT;

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

    void EditorMainWindow::closeEvent(QCloseEvent *event)
    {
        shouldClose = true;
        QtImGui::Shutdown();
        event->accept();
    }
}