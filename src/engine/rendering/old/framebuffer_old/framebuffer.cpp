#include "framebuffer.hpp"

#include "engine/rendering/shader/shader.hpp"
#include "engine/rendering/renderer/renderer.hpp"

#include <iostream>

#include "engine/core/engine.hpp"

namespace Pulse::Engine::Rendering {

    void CheckFBStatus(std::string fboName){
        
        auto* gl = Engine::Core::GetEngine().GetGL();

        GLenum status = gl->CheckFramebufferStatus(GL_FRAMEBUFFER);
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

            DEBUG_ERROR("Framebuffer error ("+ fboName +") : " + errorString);
        }
    }

    FrameBuffer::FrameBuffer(int width, int height, std::shared_ptr<Shader> shader, bool multisampled)
    {

        if(!shader){
            DEBUG_FATAL("Tried to create framebuffer with null shader !");
            return;
        }

        auto* gl = Engine::Core::GetEngine().GetGL();

        this->shader = shader;
        this->isMultisampled = multisampled;

        this->width = width;
        this->height = height;

        gl->GenFramebuffers(1, &fbo);
        gl->BindFramebuffer(GL_FRAMEBUFFER, fbo);

        if (multisampled) {
            gl->GenTextures(1, &texture);
            gl->BindTexture(GL_TEXTURE_2D_MULTISAMPLE, texture);
            gl->TexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, width, height, GL_TRUE);


            gl->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, texture, 0);

            gl->GenRenderbuffers(1, &rbo);
            gl->BindRenderbuffer(GL_RENDERBUFFER, rbo);
            gl->RenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, width, height);
            gl->FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

            GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
            gl->DrawBuffers(1, drawBuffers);

            CheckFBStatus("Multisampled fbo");

            gl->BindFramebuffer(GL_FRAMEBUFFER, 0);

            gl->GenFramebuffers(1, &resolveFBO);
            gl->BindFramebuffer(GL_FRAMEBUFFER, resolveFBO);

            gl->GenTextures(1, &resolveTexture);
            gl->BindTexture(GL_TEXTURE_2D, resolveTexture);
            gl->TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            gl->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resolveTexture, 0);

            CheckFBStatus("Resolve fbo");
        }
        else {
            gl->GenTextures(1, &texture);
            gl->BindTexture(GL_TEXTURE_2D, texture);
            gl->TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            gl->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

            gl->GenRenderbuffers(1, &rbo);
            gl->BindRenderbuffer(GL_RENDERBUFFER, rbo);
            gl->RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
            gl->FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
        }

        gl->BindFramebuffer(GL_FRAMEBUFFER, 0);
        if (multisampled)
            gl->BindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
        
        gl->BindTexture(GL_TEXTURE_2D, 0);
        gl->BindRenderbuffer(GL_RENDERBUFFER, 0);
    }


    void FrameBuffer::Shutdown()
    {
        auto* gl = Engine::Core::GetEngine().GetGL();

        gl->DeleteFramebuffers(1, &fbo);
        gl->DeleteTextures(1, &texture);
        gl->DeleteRenderbuffers(1, &rbo);
        if (resolveFBO) gl->DeleteFramebuffers(1, &resolveFBO);
        if (resolveTexture) gl->DeleteTextures(1, &resolveTexture);
    }

    GLuint FrameBuffer::GetFrameTexture()
    {
        if(isMultisampled)
            return resolveTexture;
        return texture;
    }

    void FrameBuffer::RescaleFrameBuffer(int width, int height)
    {

        auto* gl = Engine::Core::GetEngine().GetGL();

        this->width = width;
        this->height = height;

        if (isMultisampled) {
            gl->BindTexture(GL_TEXTURE_2D_MULTISAMPLE, texture);
            gl->TexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, width, height, GL_TRUE);

            gl->BindRenderbuffer(GL_RENDERBUFFER, rbo);
            gl->RenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, width, height);

            gl->BindFramebuffer(GL_FRAMEBUFFER, fbo);
            gl->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, texture, 0);
            gl->FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

            // Resize intermediate texture
            gl->BindTexture(GL_TEXTURE_2D, resolveTexture);
            gl->TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

            // Check resolveFBO status
            CheckFBStatus("Multisampled fbo (resized)");

            // Reattach resized texture to resolveFBO
            gl->BindFramebuffer(GL_FRAMEBUFFER, resolveFBO);
            gl->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resolveTexture, 0);

            // Makes sure draw buffers are set again
            GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
            gl->DrawBuffers(1, drawBuffers);

            // Check resolveFBO status
            CheckFBStatus("Resolve fbo (resized)");
        }
        else {
            gl->BindTexture(GL_TEXTURE_2D, texture);
            gl->TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            gl->BindRenderbuffer(GL_RENDERBUFFER, rbo);
            gl->RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

            gl->BindFramebuffer(GL_FRAMEBUFFER, fbo);
            gl->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
            gl->FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
        }

        // Unbind everything
        gl->BindFramebuffer(GL_FRAMEBUFFER, 0);
        if (isMultisampled)
            gl->BindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
        else
            gl->BindTexture(GL_TEXTURE_2D, 0);
        gl->BindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    void FrameBuffer::Resolve()
    {
        if (!isMultisampled) return;
        
        auto* gl = Engine::Core::GetEngine().GetGL();

        gl->BindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        gl->BindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFBO);
        gl->BlitFramebuffer(
            0, 0, width, height,
            0, 0, width, height,
            GL_COLOR_BUFFER_BIT, GL_NEAREST
        );
        gl->BindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void FrameBuffer::Draw(unsigned int VAO)
    {
        auto* gl = Engine::Core::GetEngine().GetGL();

        if (isMultisampled)
            Resolve();

        shader->Activate();

        gl->Disable(GL_DEPTH_TEST);

        gl->BindVertexArray(VAO);

        gl->ActiveTexture(GL_TEXTURE0);
        gl->BindTexture(GL_TEXTURE_2D, isMultisampled ? resolveTexture : texture);

        gl->DrawArrays(GL_TRIANGLES, 0, 6);

        gl->BindVertexArray(0);

        shader->Deactivate();
    }

    void FrameBuffer::Bind() const
    {
        Engine::Core::GetEngine().GetGL()->BindFramebuffer(GL_FRAMEBUFFER, fbo);
    }

    void FrameBuffer::Unbind() const
    {
        Engine::Core::GetEngine().GetGL()->BindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void FrameBuffer::SetShader(std::shared_ptr<Shader> shader)
    {
        this->shader = shader;
    }
    
    std::shared_ptr<Shader> FrameBuffer::GetShader()
    {
        return this->shader;
    }
}