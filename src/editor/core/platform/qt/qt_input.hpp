#pragma once

#include <unordered_map>
#include <iostream>

#include "engine/inputs/keys.hpp"
#include "engine/core/platform/iplatform.hpp"

#include <QMouseEvent>
#include <QKeyEvent>

namespace Pulse::Editor::Core::Platform
{
    class QTInput : public Engine::Core::Platform::IInput{
        public:
            void Init() override;
            void Tick() override;
            void Shutdown() override;

            // Keyboard

            /// @brief Returns true if the key is currently held down.
            /// @param key The key to query.
            /// @return True while the key is pressed; false otherwise.
            bool IsKeyDown(Engine::Input::Key key) const override;

            /// @brief Returns true if the key is currently not pressed.
            /// @param key The key to query.
            /// @return True while the key is released; false otherwise.
            bool IsKeyUp(Engine::Input::Key key) const override;

            /// @brief Returns true only on the tick/frame the key transitions from released to pressed.
            /// @param key The key to query.
            /// @return True for exactly one tick when the key is initially pressed; false if the key is held or not pressed.
            bool WasKeyPressed(Engine::Input::Key key) const override;

            /// @brief Returns true only on the tick/frame the key transitions from pressed to released.
            /// @param key The key to query.
            /// @return True for exactly one tick when the key is initially released; false if the key is held or already released.
            bool WasKeyReleased(Engine::Input::Key key) const override;

            // Mouse

            /// @brief Returns true if the mouse button is currently held down.
            /// @param button The mouse button to query.
            /// @return True while the button is pressed; false otherwise.
            bool IsMouseDown(Engine::Input::MouseButton button) const override;

            /// @brief Returns true if the mouse button is currently not pressed.
            /// @param button The mouse button to query.
            /// @return True while the button is released; false otherwise.
            bool IsMouseUp(Engine::Input::MouseButton button) const override;

            /// @brief Returns true only on the tick/frame the mouse button transitions from released to pressed.
            /// @param button The mouse button to query.
            /// @return True for exactly one tick when the button is initially pressed; false if held or not pressed.
            bool WasMousePressed(Engine::Input::MouseButton button) const override;

            /// @brief Returns true only on the tick/frame the mouse button transitions from pressed to released.
            /// @param button The mouse button to query.
            /// @return True for exactly one tick when the button is initially released; false if held or already released.
            bool WasMouseReleased(Engine::Input::MouseButton button) const override;

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
            case Engine::Input::Key::A: return Qt::Key_A;
            case Engine::Input::Key::B: return Qt::Key_B;
            case Engine::Input::Key::C: return Qt::Key_C;
            case Engine::Input::Key::D: return Qt::Key_D;
            case Engine::Input::Key::E: return Qt::Key_E;
            case Engine::Input::Key::F: return Qt::Key_F;
            case Engine::Input::Key::S: return Qt::Key_S;
            case Engine::Input::Key::W: return Qt::Key_W;
            case Engine::Input::Key::X: return Qt::Key_X;
            case Engine::Input::Key::Y: return Qt::Key_Y;
            case Engine::Input::Key::Z: return Qt::Key_Z;
            case Engine::Input::Key::Escape: return Qt::Key_Escape;
            case Engine::Input::Key::Space: return Qt::Key_Space;
            case Engine::Input::Key::F1: return Qt::Key_F1;
            case Engine::Input::Key::F2: return Qt::Key_F2;
            case Engine::Input::Key::F3: return Qt::Key_F3;
            case Engine::Input::Key::F4: return Qt::Key_F4;
            case Engine::Input::Key::F5: return Qt::Key_F5;
            case Engine::Input::Key::F6: return Qt::Key_F6;
            case Engine::Input::Key::F7: return Qt::Key_F7;
            case Engine::Input::Key::F8: return Qt::Key_F8;
            case Engine::Input::Key::F9: return Qt::Key_F9;
            case Engine::Input::Key::F10: return Qt::Key_F10;
            case Engine::Input::Key::F11: return Qt::Key_F11;
            case Engine::Input::Key::F12: return Qt::Key_F12;
            case Engine::Input::Key::Delete: return Qt::Key_Delete;
            case Engine::Input::Key::LeftControl: return Qt::Key_Control;
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
