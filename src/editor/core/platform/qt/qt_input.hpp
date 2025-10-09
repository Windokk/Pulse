#pragma once

#include <unordered_map>
#include <iostream>

#include "engine/inputs/keys.hpp"
#include "engine/debugging/debugger.hpp"
#include "engine/core/platform/iplatform.hpp"

#include <QMouseEvent>
#include <QKeyEvent>

namespace Epoch::Engine::Core::Platform
{
    class QTInput : public IInput{
        public:
            void Init() override;
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

            void KeyCallback(int key, int action);
            void MouseCallback(int button, int action);

        private:

            std::unordered_map<int, bool> mCurrentKeyState;
            std::unordered_map<int, bool> mPreviousKeyState;

            std::unordered_map<int, bool> mCurrentMouseState;
            std::unordered_map<int, bool> mPreviousMouseState;
            
    };

    static int ToQtKey(Engine::Input::Key key) {
        switch (key) {
            case Engine::Input::Key::W: return Qt::Key_W;
            case Engine::Input::Key::A: return Qt::Key_A;
            case Engine::Input::Key::S: return Qt::Key_S;
            case Engine::Input::Key::D: return Qt::Key_D;
            case Engine::Input::Key::Escape: return Qt::Key_Escape;
            case Engine::Input::Key::Space: return Qt::Key_Space;
            // TODO: Add other keys as needed
            default: return Qt::Key_unknown;
        }
    }

    static Qt::MouseButton ToQtMouseButton(Engine::Input::MouseButton button) {
        switch (button) {
            case Engine::Input::MouseButton::Left: return Qt::LeftButton;
            case Engine::Input::MouseButton::Right: return Qt::RightButton;
            case Engine::Input::MouseButton::Middle: return Qt::MiddleButton;
            // Add others if needed
            default: return Qt::NoButton;
        }
    }
    
}
