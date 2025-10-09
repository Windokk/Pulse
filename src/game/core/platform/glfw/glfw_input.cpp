#include "engine/core/engine.hpp"

#include "glfw_window.hpp"

namespace Epoch::Engine::Core::Platform{

    void GLFWInput::SetWindow(GLFWwindow * window)
    {
        win = window;
    }

    void GLFWInput::Init() {
        mCurrentKeyState.clear();
        mPreviousKeyState.clear();
        mCurrentMouseState.clear();
        mPreviousMouseState.clear();

        glfwSetKeyCallback(win, [](GLFWwindow* w, int key, int sc, int action, int mods) {
            auto* window = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(w));
            if (window) {
                window->inputManager->KeyCallback(key, action);
            }
        });

        glfwSetMouseButtonCallback(win, [](GLFWwindow* w, int button, int action, int mods) {
            auto* window = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(w));
            if (window) {
                window->inputManager->MouseCallback(button, action);
            }
        });
    }

    void GLFWInput::Tick()
    {
        mPreviousKeyState = mCurrentKeyState;
        mPreviousMouseState = mCurrentMouseState;
    }

    void GLFWInput::Shutdown()
    {
        mCurrentKeyState.clear();
        mPreviousKeyState.clear();
        mCurrentMouseState.clear();
        mPreviousMouseState.clear();
    }

    void GLFWInput::KeyCallback(int key, int action)
    {
        if (key >= 0) {
            if (action == GLFW_PRESS || action == GLFW_REPEAT) {
                mCurrentKeyState[key] = true;
            } else if (action == GLFW_RELEASE) {
                mCurrentKeyState[key] = false;
            }
        }
    }

    void GLFWInput::MouseCallback(int button, int action)
    {
        if (button >= 0) {
            if (action == GLFW_PRESS || action == GLFW_REPEAT) {
                mCurrentMouseState[button] = true;
            } else if (action == GLFW_RELEASE) {
                mCurrentMouseState[button] = false;
            }
        }
    }

    bool GLFWInput::IsKeyDown(Input::Key key) const {
        auto it = mCurrentKeyState.find(ToGLFWKey(key));
        return it != mCurrentKeyState.end() && it->second;
    }

    bool GLFWInput::IsKeyUp(Input::Key key) const {
        return !IsKeyDown(key);
    }

    bool GLFWInput::WasKeyPressed(Input::Key key) const {
        int glfwKey = ToGLFWKey(key);
        bool prev = false, curr = false;
        
        auto prevIt = mPreviousKeyState.find(glfwKey);
        if (prevIt != mPreviousKeyState.end()) prev = prevIt->second;

        auto currIt = mCurrentKeyState.find(glfwKey);
        if (currIt != mCurrentKeyState.end()) curr = currIt->second;

        return !prev && curr;
    }

    bool GLFWInput::WasKeyReleased(Input::Key key) const {
        int glfwKey = ToGLFWKey(key);

        bool prev = false, curr = false;

        auto prevIt = mPreviousKeyState.find(glfwKey);
        if (prevIt != mPreviousKeyState.end()) prev = prevIt->second;

        auto currIt = mCurrentKeyState.find(glfwKey);
        if (currIt != mCurrentKeyState.end()) curr = currIt->second;

        return prev && !curr;
    }

    bool GLFWInput::IsMouseDown(Input::MouseButton button) const
    {
        auto it = mCurrentMouseState.find(ToGLFWMouseButton(button));
        return it != mCurrentMouseState.end() && it->second;
    }

    bool GLFWInput::IsMouseUp(Input::MouseButton button) const
    {
        return !IsMouseDown(button);
    }

    bool GLFWInput::WasMousePressed(Input::MouseButton button) const
    {
        int glfwButton = ToGLFWMouseButton(button);

        bool prev = false, curr = false;

        auto prevIt = mPreviousMouseState.find(glfwButton);
        if (prevIt != mPreviousMouseState.end()) prev = prevIt->second;

        auto currIt = mCurrentMouseState.find(glfwButton);
        if (currIt != mCurrentMouseState.end()) curr = currIt->second;

        return !prev && curr;
    }

    bool GLFWInput::WasMouseReleased(Input::MouseButton button) const
    {
        int glfwButton = ToGLFWMouseButton(button);

        bool prev = false, curr = false;

        auto prevIt = mPreviousMouseState.find(glfwButton);
        if (prevIt != mPreviousMouseState.end()) prev = prevIt->second;

        auto currIt = mCurrentMouseState.find(glfwButton);
        if (currIt != mCurrentMouseState.end()) curr = currIt->second;

        return prev && !curr;
    }
    
    void GLFWInput::SetCursorVisibility(bool visible) const
    {
        glfwSetInputMode(win, GLFW_CURSOR, visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
    }

    void GLFWInput::GetCursorPos(double *x, double *y) const
    {
        glfwGetCursorPos(win, x, y);
    }

    void GLFWInput::SetCursorPos(double x, double y) const
    {
        glfwSetCursorPos(win, x, y);
    }
};