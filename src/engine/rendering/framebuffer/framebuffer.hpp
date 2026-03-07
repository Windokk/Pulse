#pragma once

#include "engine/rendering/utils.hpp"

#include <memory>

namespace Pulse::Engine::Rendering {
    
    class Shader;

    class FrameBuffer{
    public:
        FrameBuffer(int width, int height, std::shared_ptr<Shader> shader, bool multisampled = true);
        void Shutdown();
        GLuint GetFrameTexture();
        void RescaleFrameBuffer(int width, int height);
        void Resolve();
        void Draw(unsigned int VAO);
        void Bind() const;
        void Unbind() const;
        void SetShader(std::shared_ptr<Shader> shader);
        std::shared_ptr<Shader> GetShader();
        bool isMultisampled = false;
        int GetWidth() const { return width; }
        int GetHeight() const { return height; }
        GLuint GetFBO() const { return fbo; }
    private:
        GLuint fbo;
        GLuint texture;
        GLuint rbo;
        std::shared_ptr<Shader> shader;

        GLuint resolveFBO = 0;
        GLuint resolveTexture = 0;
        int width = 0;
        int height = 0;

    };
}