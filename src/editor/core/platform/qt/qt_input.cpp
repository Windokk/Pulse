#include "qt_input.hpp"

#include <QGuiApplication>
#include <QCursor>

#include "engine/debugging/logger.hpp"

#include "editor/gui/main_win.hpp"

#include "engine/core/engine.hpp"

namespace Pulse::Editor::Core::Platform{
    
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

    bool QTInput::IsKeyDown(Engine::Input::Key key) const {
        auto it = mCurrentKeyState.find(ToQtKey(key));
        return it != mCurrentKeyState.end() && it->second;
    }

    bool QTInput::IsKeyUp(Engine::Input::Key key) const {
        return !IsKeyDown(key);
    }

    bool QTInput::WasKeyPressed(Engine::Input::Key key) const {
        int qtKey = ToQtKey(key);
        bool currDown = IsKeyDown(key);

        // Previous state: false if missing or explicitly false
        bool prevDown = false;
        auto it = mPreviousKeyState.find(qtKey);
        if (it != mPreviousKeyState.end()) prevDown = it->second;

        return currDown && !prevDown;
    }

    bool QTInput::WasKeyReleased(Engine::Input::Key key) const {
        int qtKey = ToQtKey(key);
        bool currDown = IsKeyDown(key);

        bool prevDown = false;
        auto it = mPreviousKeyState.find(qtKey);
        if (it != mPreviousKeyState.end()) prevDown = it->second;

        return !currDown && prevDown;
    }

    bool QTInput::IsMouseDown(Engine::Input::MouseButton button) const
    {
        auto it = mCurrentMouseState.find(static_cast<int>(ToQtMouseButton(button)));
        return it != mCurrentMouseState.end() && it->second;
    }

    bool QTInput::IsMouseUp(Engine::Input::MouseButton button) const
    {
        return !IsMouseDown(button);
    }

    bool QTInput::WasMousePressed(Engine::Input::MouseButton button) const
    {
        int btn = static_cast<int>(ToQtMouseButton(button));
        auto prevIt = mPreviousMouseState.find(btn);
        auto currIt = mCurrentMouseState.find(btn);
        
        bool prev = (prevIt != mPreviousMouseState.end()) ? prevIt->second : false;
        bool curr = (currIt != mCurrentMouseState.end()) ? currIt->second : false;
        
        return !prev && curr;
    }

    bool QTInput::WasMouseReleased(Engine::Input::MouseButton button) const
    {
        int btn = ToQtMouseButton(button);
        auto prevIt = mPreviousMouseState.find(btn);
        auto currIt = mCurrentMouseState.find(btn);

        bool prev = (prevIt != mPreviousMouseState.end()) ? prevIt->second : false;
        bool curr = (currIt != mCurrentMouseState.end()) ? currIt->second : false;

        return prev && !curr;
    }


    void QTInput::SetCursorVisibility(bool visible) const {

        EditorMainWindow* win = dynamic_cast<EditorMainWindow*>(Engine::Core::GetEngine().GetWindow());

        if(win != nullptr){
            if (visible) {
                
                win->unsetCursor();
            }
            else {
                
                win->setCursor(Qt::BlankCursor);
            }
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