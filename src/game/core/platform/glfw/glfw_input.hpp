#pragma once

#include <unordered_map>
#include <iostream>

#include "engine/inputs/keys.hpp"
#include "engine/debugging/debugger.hpp"
#include "engine/core/platform/iplatform.hpp"

#include <GLFW/glfw3.h>

namespace Epoch::Engine::Core::Platform
{
    class GLFWInput : public IInput{
        public:
            void Init() override;
            void SetWindow(GLFWwindow* window);
            void Tick() override;
            void Shutdown() override;

            // Keyboard
            bool IsKeyDown(Input::Key key) const override;
            bool IsKeyUp(Input::Key key) const override;
            bool WasKeyPressed(Input::Key key) const override;
            bool WasKeyReleased(Input::Key key) const override;

            // Mouse
            bool IsMouseDown(Input::MouseButton button) const override;
            bool IsMouseUp(Input::MouseButton button) const override;
            bool WasMousePressed(Input::MouseButton button) const override;
            bool WasMouseReleased(Input::MouseButton button) const override;

            void SetCursorVisibility(bool visible) const override;
            void GetCursorPos(double* x, double* y) const override;
            void SetCursorPos(double x, double y) const override;


        private:

            void KeyCallback(int key, int action);
            void MouseCallback(int button, int action);

            std::unordered_map<int, bool> mCurrentKeyState;
            std::unordered_map<int, bool> mPreviousKeyState;

            std::unordered_map<int, bool> mCurrentMouseState;
            std::unordered_map<int, bool> mPreviousMouseState;
            
            GLFWwindow* win = nullptr;
    };

    static int ToGLFWKey(Engine::Input::Key key) {
        switch (key) {
            case Engine::Input::Key::W: return GLFW_KEY_W;
            case Engine::Input::Key::A: return GLFW_KEY_A;
            case Engine::Input::Key::S: return GLFW_KEY_S;
            case Engine::Input::Key::D: return GLFW_KEY_D;
            case Engine::Input::Key::Escape: return GLFW_KEY_ESCAPE;
            case Engine::Input::Key::Space: return GLFW_KEY_SPACE;
            default: return GLFW_KEY_UNKNOWN;
        }
    }

    static int ToGLFWMouseButton(Engine::Input::MouseButton button) {
        switch (button) {
            case Engine::Input::MouseButton::Left: return GLFW_MOUSE_BUTTON_LEFT;
            case Engine::Input::MouseButton::Right: return GLFW_MOUSE_BUTTON_RIGHT;
            case Engine::Input::MouseButton::Middle: return GLFW_MOUSE_BUTTON_MIDDLE;
            default: return -1;
        }
    }
}
