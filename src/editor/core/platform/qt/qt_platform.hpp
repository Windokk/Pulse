#pragma once

#include "engine/core/platform/iplatform.hpp"

#include <memory>

#include "qt_input.hpp"
#include "qt_window.hpp"

#ifdef CreateWindow
#undef CreateWindow
#endif

namespace Epoch::Engine::Core::Platform {

    class QTPlatform : public IPlatform {
        public:
            IWindow* GetWindow() override { return window.get(); };
            IInput* GetInput() override { return input.get(); };

            void CreateWindow(const std::string& title, const int& width, const int& height, const bool& fullscreen, const int& vsync) override {
                window = std::make_unique<QTWindow>();
                window->Init(title, width, height, fullscreen, vsync);
            }

            void CreateInput() override {
                input = std::make_unique<QTInput>();
                input->Init();
            }

            // Utility
            void SetClipboardText(const std::string& text) override { }
            std::string GetClipboardText() override { return ""; }
        private:
            std::unique_ptr<IWindow> window;
            std::unique_ptr<IInput> input;
    };
}