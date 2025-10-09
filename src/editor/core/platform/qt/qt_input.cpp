#include "qt_input.hpp"

#include <QGuiApplication>
#include <QCursor>

namespace Epoch::Engine::Core::Platform{
    
    void QTInput::Init()
    {
        
    }

    void QTInput::Tick() {
        mPreviousKeyState = mCurrentKeyState;
        mPreviousMouseState = mCurrentMouseState;
    }

    void QTInput::Shutdown()
    {

    }

    void QTInput::KeyCallback(int key, int action) {
        bool isDown = (action != 0);
        mCurrentKeyState[key] = isDown;
    }

    void QTInput::MouseCallback(int button, int action) {
        bool isDown = (action != 0);
        mCurrentMouseState[button] = isDown;
    }

    bool QTInput::IsKeyDown(Input::Key key) const {
        auto it = mCurrentKeyState.find(ToQtKey(key));
        return it != mCurrentKeyState.end() && it->second;
    }

    bool QTInput::IsKeyUp(Input::Key key) const {
        return !IsKeyDown(key);
    }

    bool QTInput::WasKeyPressed(Input::Key key) const {
        int qtKey = ToQtKey(key);
        bool currDown = IsKeyDown(key);

        // Previous state: false if missing or explicitly false
        bool prevDown = false;
        auto it = mPreviousKeyState.find(qtKey);
        if (it != mPreviousKeyState.end()) prevDown = it->second;

        return currDown && !prevDown;
    }

    bool QTInput::WasKeyReleased(Input::Key key) const {
        int qtKey = ToQtKey(key);
        bool currDown = IsKeyDown(key);

        bool prevDown = false;
        auto it = mPreviousKeyState.find(qtKey);
        if (it != mPreviousKeyState.end()) prevDown = it->second;

        return !currDown && prevDown;
    }

    bool QTInput::IsMouseDown(Input::MouseButton button) const
    {
        auto it = mCurrentMouseState.find(static_cast<int>(ToQtMouseButton(button)));
        return it != mCurrentMouseState.end() && it->second;
    }

    bool QTInput::IsMouseUp(Input::MouseButton button) const
    {
        return !IsMouseDown(button);
    }

    bool QTInput::WasMousePressed(Input::MouseButton button) const
    {
        int btn = static_cast<int>(ToQtMouseButton(button));
        auto prevIt = mPreviousMouseState.find(btn);
        auto currIt = mCurrentMouseState.find(btn);
        
        bool prev = (prevIt != mPreviousMouseState.end()) ? prevIt->second : false;
        bool curr = (currIt != mCurrentMouseState.end()) ? currIt->second : false;
        
        return !prev && curr;
    }

    bool QTInput::WasMouseReleased(Input::MouseButton button) const
    {
        int btn = ToQtMouseButton(button);
        auto prevIt = mPreviousMouseState.find(btn);
        auto currIt = mCurrentMouseState.find(btn);

        bool prev = (prevIt != mPreviousMouseState.end()) ? prevIt->second : false;
        bool curr = (currIt != mCurrentMouseState.end()) ? currIt->second : false;

        return prev && !curr;
    }


    void QTInput::SetCursorVisibility(bool visible) const {
        if (visible) {
            QGuiApplication::restoreOverrideCursor();
        } else {
            QGuiApplication::setOverrideCursor(Qt::BlankCursor);
        }
    }

    void QTInput::GetCursorPos(double* x, double* y) const {
        QPoint pos = QCursor::pos();
        *x = pos.x();
        *y = pos.y();
    }

    void QTInput::SetCursorPos(double x, double y) const {
        QCursor::setPos(QPoint(int(x), int(y)));
    }
}