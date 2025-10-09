#pragma once

#include "engine/core/platform/iwindow.hpp"
#include "engine/rendering/renderer/renderer.hpp"
#include "engine/debugging/debugger.hpp"
#include "qt_input.hpp"

#include <QWindow>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QGuiApplication>
#include <QScreen>
#include <QtGui>
#include <vector>


namespace Epoch::Engine::Core::Platform {

    class QTWindow : public QWindow, public IWindow {
        Q_OBJECT

    public:
        QTWindow();
        ~QTWindow() override;

        bool event(QEvent *e);

        void Init(const std::string& title, const int& width, const int& height,
                  const bool& fullscreen, const int& vsync) override;

        void PollEvents() override;
        void SwapBuffers() override;
        bool ShouldClose() const override;

        int GetFramebufferWidth() const override;
        int GetFramebufferHeight() const override;

        void* GetNativeHandle() const override;

        void Destroy() const override;
        void ToggleFullscreen() override;

        SystemInfos GetSystemInfos() const override;

        void SetQTInputManager(QTInput* inputManager) {
            this->inputManager = inputManager;
        }

        void SetTitle(const std::string& title){
            this->setTitle(QString::fromStdString(title));
        }

    protected:
        void resizeEvent(QResizeEvent* event) override;
        void closeEvent(QCloseEvent* event) override;

    private:
        QOpenGLContext* context = nullptr;
        QOpenGLFunctions* gl = nullptr;
        QTInput* inputManager = nullptr;

        bool shouldClose = false;
        bool isFullscreen = false;

        QRect windowedGeometry; // For restoring window after fullscreen

        bool initialized = false;
    };

}