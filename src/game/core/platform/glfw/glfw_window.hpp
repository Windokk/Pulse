#pragma once

#include "engine/core/platform/iwindow.hpp"

#include "engine/rendering/renderer/renderer.hpp"

#include "engine/debugging/debugger.hpp"

#include "glfw_input.hpp"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace Epoch::Engine::Core::Platform {

    inline void OnWindowResize(GLFWwindow *window, int width, int height)
    {
        Rendering::Renderer::GetInstance().RescaleFramebuffers(width, height);
    }

    class GLFWWindow : public IWindow {
    public:
        void Init(const std::string& title, const int& width, const int& height, 
                    const bool& fullscreen, const int& vsync) override {
            
            glfwInit();
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_SAMPLES, 4);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

            if(fullscreen){
                GLFWmonitor* primary = glfwGetPrimaryMonitor();
                const GLFWvidmode* mode = glfwGetVideoMode(primary);
                window = glfwCreateWindow(mode->width, mode->height, title.c_str(), primary, nullptr);
            }
            else{
                window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
            }
            
            if (window == NULL)
            {
                glfwTerminate();
                DEBUG_FATAL("Failed to create GLFW window");
            }

            glfwMakeContextCurrent(window);
            glfwSetWindowUserPointer(window, this);
            glfwSetWindowSizeCallback(window, OnWindowResize);
            glfwSwapInterval(vsync);

            gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

            GetGL().InitFromGLAD();
        }

        void SetGLFWInputManager(GLFWInput* inputManager){
            this->inputManager = inputManager;
        }

        void SetTitle(const std::string& title) override {
            glfwSetWindowTitle(window, title.c_str());
        }

        void PollEvents() override {
            glfwPollEvents();
        }

        void SwapBuffers() override {
            glfwSwapBuffers(window);
        }

        bool ShouldClose() const override {
            return glfwWindowShouldClose(window);
        }

        int GetFramebufferWidth() const override{
            int width;
            glfwGetFramebufferSize(window, &width, nullptr);
            return width;
        }

        int GetFramebufferHeight() const override {
            int height;
            glfwGetFramebufferSize(window, nullptr, &height);
            return height;
        }

        void* GetNativeHandle() const override {
            return static_cast<void*>(window);
        }

        void Destroy() const override{
            glfwDestroyWindow(window);
            glfwTerminate();
        }
  
        void ToggleFullscreen() override
        {
            const bool fullscreen = glfwGetWindowMonitor(window) != nullptr;
            if(fullscreen) {
                // Restore the window position and size.
                glfwSetWindowMonitor(window, nullptr, windowPosX, windowPosY, windowWidth, windowHeight, 0);
                // Check the window position and size (if we are on a screen smaller than the initial size).
                glfwGetWindowPos(window, &windowPosX, &windowPosY);
                glfwGetWindowSize(window, &windowWidth, &windowHeight);
                Rendering::Renderer::GetInstance().RescaleFramebuffers(windowWidth, windowHeight);
            } else {
                // Backup the window current frame.
                glfwGetWindowPos(window, &windowPosX, &windowPosY);
                glfwGetWindowSize(window, &windowWidth, &windowHeight);
                // Move to fullscreen on the primary monitor.
                GLFWmonitor * monitor	= glfwGetPrimaryMonitor();
                const GLFWvidmode * mode = glfwGetVideoMode(monitor);
                glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
                Rendering::Renderer::GetInstance().RescaleFramebuffers(mode->width, mode->height);
            }
        }

        SystemInfos GetSystemInfos() const override{
            SystemInfos ret{0};
            
            const GLubyte* renderer = glGetString(GL_RENDERER);     // GPU
            const GLubyte* vendor   = glGetString(GL_VENDOR);       // GPU vendor
            const GLubyte* version  = glGetString(GL_VERSION);      // OpenGL version

            ret.gpu_vendor = reinterpret_cast<const char*>(vendor);
            ret.gpu_renderer = reinterpret_cast<const char*>(renderer);
            ret.gl_version = reinterpret_cast<const char*>(version);

            // GLFW context version
            int major, minor, rev;
            glfwGetVersion(&major, &minor, &rev);
            ret.windowHostVersion = std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(rev);

            // Monitor and resolution
            int count = 0;
            GLFWmonitor** monitors = glfwGetMonitors(&count);
            ret.connectedMonitorsCount = count;

            for (int i = 0; i < count; ++i) {
                const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);
                ret.monitors.push_back({mode->width, mode->height, mode->refreshRate});
            }
            
            ret.windowHost = GLFW;

            return ret;
        }

        GLFWInput* inputManager;

    private:
        GLFWwindow* window = nullptr;

        int windowPosX = 0;
        int windowPosY = 0;
        int windowWidth = 0;
        int windowHeight = 0;
    };
}