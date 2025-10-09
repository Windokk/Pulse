#include "framebuffer.hpp"

#include "engine/rendering/shader/shader.hpp"
#include "engine/rendering/renderer/renderer.hpp"

#include <iostream>


namespace Epoch::Engine::Rendering {

    FrameBuffer::FrameBuffer(int width, int height, std::shared_ptr<Shader> shader, bool multisampled)
    {
        this->shader = shader;
        this->isMultisampled = multisampled;

        this->width = width;
        this->height = height;

        GetGL().GenFramebuffers(1, &fbo);
        GetGL().BindFramebuffer(GL_FRAMEBUFFER, fbo);

        if (multisampled) {
            GetGL().GenTextures(1, &texture);
            GetGL().BindTexture(GL_TEXTURE_2D_MULTISAMPLE, texture);
            GetGL().TexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, width, height, GL_TRUE);


            GetGL().FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, texture, 0);

            GetGL().GenRenderbuffers(1, &rbo);
            GetGL().BindRenderbuffer(GL_RENDERBUFFER, rbo);
            GetGL().RenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, width, height);
            GetGL().FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

            GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
            GetGL().DrawBuffers(1, drawBuffers);

            GLenum status = GetGL().CheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE) {
                std::string errorString;

                switch (status) {
                    case GL_FRAMEBUFFER_UNDEFINED:
                        errorString = "GL_FRAMEBUFFER_UNDEFINED";
                        break;
                    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                        errorString = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
                        break;
                    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                        errorString = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
                        break;
                    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
                        errorString = "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
                        break;
                    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
                        errorString = "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
                        break;
                    case GL_FRAMEBUFFER_UNSUPPORTED:
                        errorString = "GL_FRAMEBUFFER_UNSUPPORTED";
                        break;
                    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
                        errorString = "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
                        break;
                    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
                        errorString = "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
                        break;
                    default:
                        errorString = "Unknown error (code: " + std::to_string(status) + ")";
                        break;
                }

                DEBUG_ERROR("Failed to create framebuffer: " + errorString);
            }

            GetGL().BindFramebuffer(GL_FRAMEBUFFER, 0);

            GetGL().GenFramebuffers(1, &resolveFBO);
            GetGL().BindFramebuffer(GL_FRAMEBUFFER, resolveFBO);

            GetGL().GenTextures(1, &resolveTexture);
            GetGL().BindTexture(GL_TEXTURE_2D, resolveTexture);
            GetGL().TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            GetGL().TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            GetGL().TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            GetGL().TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            GetGL().TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            GetGL().FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resolveTexture, 0);

            status = GetGL().CheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE) {
                std::string errorString;

                switch (status) {
                    case GL_FRAMEBUFFER_UNDEFINED:
                        errorString = "GL_FRAMEBUFFER_UNDEFINED";
                        break;
                    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                        errorString = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
                        break;
                    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                        errorString = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
                        break;
                    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
                        errorString = "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
                        break;
                    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
                        errorString = "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
                        break;
                    case GL_FRAMEBUFFER_UNSUPPORTED:
                        errorString = "GL_FRAMEBUFFER_UNSUPPORTED";
                        break;
                    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
                        errorString = "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
                        break;
                    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
                        errorString = "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
                        break;
                    default:
                        errorString = "Unknown error (code: " + std::to_string(status) + ")";
                        break;
                }

                DEBUG_ERROR("Failed to create framebuffer: " + errorString);
            }

        }
        else {
            GetGL().GenTextures(1, &texture);
            GetGL().BindTexture(GL_TEXTURE_2D, texture);
            GetGL().TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            GetGL().TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            GetGL().TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            GetGL().TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            GetGL().TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            GetGL().FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

            GetGL().GenRenderbuffers(1, &rbo);
            GetGL().BindRenderbuffer(GL_RENDERBUFFER, rbo);
            GetGL().RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
            GetGL().FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
        }

        GetGL().BindFramebuffer(GL_FRAMEBUFFER, 0);
        if (multisampled)
            GetGL().BindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
        
        GetGL().BindTexture(GL_TEXTURE_2D, 0);
        GetGL().BindRenderbuffer(GL_RENDERBUFFER, 0);
    }


    void FrameBuffer::Shutdown()
    {
        GetGL().DeleteFramebuffers(1, &fbo);
        GetGL().DeleteTextures(1, &texture);
        GetGL().DeleteRenderbuffers(1, &rbo);
        if (resolveFBO) GetGL().DeleteFramebuffers(1, &resolveFBO);
        if (resolveTexture) GetGL().DeleteTextures(1, &resolveTexture);
    }

    GLuint FrameBuffer::GetFrameTexture()
    {
        if(isMultisampled)
            return resolveTexture;
        return texture;
    }

    void FrameBuffer::RescaleFrameBuffer(int width, int height)
    {
        this->width = width;
        this->height = height;

        if (isMultisampled) {
            GetGL().BindTexture(GL_TEXTURE_2D_MULTISAMPLE, texture);
            GetGL().TexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, width, height, GL_TRUE);
            // No texture parameters needed for multisampled textures generally

            GetGL().BindRenderbuffer(GL_RENDERBUFFER, rbo);
            GetGL().RenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, width, height);

            GetGL().BindFramebuffer(GL_FRAMEBUFFER, fbo);
            GetGL().FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, texture, 0);
            GetGL().FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

            // Resize intermediate texture
            GetGL().BindTexture(GL_TEXTURE_2D, resolveTexture);
            GetGL().TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

            // Reattach resized texture to resolveFBO
            GetGL().BindFramebuffer(GL_FRAMEBUFFER, resolveFBO);
            GetGL().FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resolveTexture, 0);

            // Makes sure draw buffers are set again
            GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
            GetGL().DrawBuffers(1, drawBuffers);

            // Check resolveFBO status
            if (GetGL().CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                DEBUG_ERROR("Resolve framebuffer is incomplete after resize!");
            }
        }
        else {
            GetGL().BindTexture(GL_TEXTURE_2D, texture);
            GetGL().TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            GetGL().TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            GetGL().TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            GetGL().TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            GetGL().TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            GetGL().BindRenderbuffer(GL_RENDERBUFFER, rbo);
            GetGL().RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

            GetGL().BindFramebuffer(GL_FRAMEBUFFER, fbo);
            GetGL().FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
            GetGL().FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
        }

        // Unbind everything
        GetGL().BindFramebuffer(GL_FRAMEBUFFER, 0);
        if (isMultisampled)
            GetGL().BindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
        else
            GetGL().BindTexture(GL_TEXTURE_2D, 0);
        GetGL().BindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    void FrameBuffer::Resolve()
    {
        if (!isMultisampled) return;

        GetGL().BindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        GetGL().BindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFBO);
        GetGL().BlitFramebuffer(
            0, 0, width, height,
            0, 0, width, height,
            GL_COLOR_BUFFER_BIT, GL_NEAREST
        );
        GetGL().BindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void FrameBuffer::Draw(unsigned int VAO)
    {
        Resolve();

        shader->Activate();
        GetGL().BindVertexArray(VAO);
        GetGL().Disable(GL_DEPTH_TEST);

        GetGL().ActiveTexture(GL_TEXTURE0);
        if (isMultisampled) {
            GetGL().BindTexture(GL_TEXTURE_2D, resolveTexture);
        } else {
            GetGL().BindTexture(GL_TEXTURE_2D, texture);
        }

        GetGL().DrawArrays(GL_TRIANGLES, 0, 6); 

        GetGL().BindVertexArray(0);

        for(int i = 0; i < 4; i++){
            GetGL().ActiveTexture(GL_TEXTURE0 + i);
            GetGL().BindTexture(GL_TEXTURE_2D, 0);
        }

        shader->Deactivate();

        // Unbind framebuffers
        GetGL().BindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void FrameBuffer::Bind() const
    {
        GetGL().BindFramebuffer(GL_FRAMEBUFFER, fbo);
    }

    void FrameBuffer::Unbind() const
    {
        GetGL().BindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void FrameBuffer::SetShader(std::shared_ptr<Shader> shader)
    {
        this->shader = shader;
    }
}