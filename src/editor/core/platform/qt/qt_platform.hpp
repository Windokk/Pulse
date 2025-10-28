#pragma once

#include "engine/core/platform/iplatform.hpp"

#include <memory>

#include "qt_input.hpp"

#include "editor/gui/main_win.hpp"

#ifdef CreateWindow
#undef CreateWindow
#endif

namespace Epoch::Editor::Core::Platform {

    class QTPlatform : public Engine::Core::Platform::IPlatform {
        public:
            Engine::Core::Platform::IWindow* GetWindow() override { return window.get(); };
            Engine::Core::Platform::IInput* GetInput() override { return input.get(); };
            
            void CreateWindow(const std::string& title, const int& width, const int& height, const bool& fullscreen, const int& vsync) override {
                window = std::make_unique<EditorMainWindow>();
                window->Init(title, width, height, fullscreen, vsync);

                for (int i = 0; i < 10; ++i)
                    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
            }

            void CreateInput() override {
                input = std::make_unique<QTInput>();
                input->Init();
                
                auto qtWindow = dynamic_cast<EditorMainWindow*>(window.get());
                if (qtWindow) {
                    qtWindow->SetQTInputManager(static_cast<QTInput*>(input.get()));
                }
            }

            // Utility
            void SetClipboardText(const std::string& text) override { }
            std::string GetClipboardText() override { return ""; }
        private:
            std::unique_ptr<Engine::Core::Platform::IWindow> window;
            std::unique_ptr<Engine::Core::Platform::IInput> input;
    };
}