#pragma once

#include <string>
#include <vector>

#include "engine/inputs/keys.hpp"

namespace Epoch::Engine::Core::Platform {

    class IInput {
    public:
        virtual ~IInput() = default;
        virtual void Init() = 0;
        virtual void Tick() = 0;
        virtual void Shutdown() = 0;

        // Keyboard
        virtual bool IsKeyDown(Input::Key key) const = 0;
        virtual bool IsKeyUp(Input::Key key) const = 0;
        virtual bool WasKeyPressed(Input::Key key) const = 0;
        virtual bool WasKeyReleased(Input::Key key) const = 0;

        // Mouse/Cursor
        virtual bool IsMouseDown(Input::MouseButton button) const = 0;
        virtual bool IsMouseUp(Input::MouseButton button) const = 0;
        virtual bool WasMousePressed(Input::MouseButton button) const = 0;
        virtual bool WasMouseReleased(Input::MouseButton button) const = 0;

        virtual void SetCursorVisibility(bool visible) const = 0;
        virtual void GetCursorPos(double* x, double* y) const = 0;
        virtual void SetCursorPos(double x, double y) const = 0;
    };

}