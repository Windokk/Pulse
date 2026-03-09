#include "gl_framebuffer.hpp"

#include "engine/rendering/shader/shader.hpp"
#include "engine/rendering/backends/opengl/gl_utils.hpp"

namespace Pulse::Engine::Rendering{
    
    void GLFramebuffer::CheckFBStatus(){

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
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

            DEBUG_ERROR("Framebuffer error (", m_FBO,") : " ,errorString);
        }
    }

    GLFramebuffer::GLFramebuffer(const FramebufferSpecifications &spec)
    {
        m_Specifications = spec;
        
        glGenFramebuffers(1, &m_FBO);
        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

        if (spec.multisampled) {
            glGenTextures(1, &m_ColorAttachment);
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_ColorAttachment);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, spec.width, spec.height, GL_TRUE);


            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_ColorAttachment, 0);

            glGenRenderbuffers(1, &m_RBO);
            glBindRenderbuffer(GL_RENDERBUFFER, m_RBO);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, spec.width, spec.height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_RBO);

            GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
            glDrawBuffers(1, drawBuffers);

            CheckFBStatus();

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            glGenFramebuffers(1, &m_ResolveFBO);
            glBindFramebuffer(GL_FRAMEBUFFER, m_ResolveFBO);

            glGenTextures(1, &m_ResolveTexture);
            glBindTexture(GL_TEXTURE_2D, m_ResolveTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, spec.width, spec.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ResolveTexture, 0);

            CheckFBStatus();
        }
        else {
            glGenTextures(1, &m_ColorAttachment);
            glBindTexture(GL_TEXTURE_2D, m_ColorAttachment);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, spec.width, spec.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachment, 0);

            glGenRenderbuffers(1, &m_RBO);
            glBindRenderbuffer(GL_RENDERBUFFER, m_RBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, spec.width, spec.height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_RBO);

            CheckFBStatus();
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (spec.multisampled)
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
        
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    GLFramebuffer::~GLFramebuffer()
    {
        glDeleteFramebuffers(1, &m_FBO);
        glDeleteTextures(1, &m_ColorAttachment);
        glDeleteRenderbuffers(1, &m_RBO);
        if (m_ResolveFBO) glDeleteFramebuffers(1, &m_ResolveFBO);
        if (m_ResolveTexture) glDeleteTextures(1, &m_ResolveTexture);
    }

    void GLFramebuffer::Bind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    }

    void GLFramebuffer::Unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GLFramebuffer::ResolveMultisampled()
    {
        if (!m_Specifications.multisampled) return;

        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_ResolveFBO);
        glBlitFramebuffer(
            0, 0, m_Specifications.width, m_Specifications.height,
            0, 0, m_Specifications.width, m_Specifications.height,
            GL_COLOR_BUFFER_BIT, GL_NEAREST
        );
        glBindFramebuffer(GL_FRAMEBUFFER, 0);    
    }

    void GLFramebuffer::Resize(uint32_t width, uint32_t height)
    {
        m_Specifications.width = width;
        m_Specifications.height = height;

        if (m_Specifications.multisampled) {
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_ColorAttachment);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, width, height, GL_TRUE);

            glBindRenderbuffer(GL_RENDERBUFFER, m_FBO);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, width, height);

            glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_ColorAttachment, 0);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_RBO);

            // Resize intermediate texture
            glBindTexture(GL_TEXTURE_2D, m_ResolveTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

            // Check resolveFBO status
            CheckFBStatus();

            // Reattach resized texture to resolveFBO
            glBindFramebuffer(GL_FRAMEBUFFER, m_ResolveFBO);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ResolveTexture, 0);

            // Makes sure draw buffers are set again
            GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
            glDrawBuffers(1, drawBuffers);

            // Check resolveFBO status
            CheckFBStatus();
        }
        else {
            glBindTexture(GL_TEXTURE_2D, m_ColorAttachment);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glBindRenderbuffer(GL_RENDERBUFFER, m_RBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

            glBindFramebuffer(GL_FRAMEBUFFER, m_RBO);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachment, 0);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_RBO);
        }

        // Unbind everything
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (m_Specifications.multisampled)
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
        else
            glBindTexture(GL_TEXTURE_2D, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    uint32_t GLFramebuffer::GetColorAttachment() const
    {
        return m_ColorAttachment;
    }
}