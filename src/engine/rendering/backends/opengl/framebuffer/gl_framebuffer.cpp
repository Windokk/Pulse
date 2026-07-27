#include "gl_framebuffer.hpp"

#include "engine/rendering/shader/shader.hpp"
#include "engine/rendering/backends/opengl/gl_utils.hpp"

#include "engine/debugging/logger.hpp"

#include "engine/rendering/backends/opengl/texture/gl_texture.hpp"

namespace Pulse::Engine::Rendering{
    
    void GLFramebuffer::CheckFBStatus(){

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE && (m_Specifications.hasColor || m_Specifications.hasDepth)) {
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
            
            if(spec.hasColor)
            {
                glGenTextures(1, &m_ColorAttachment);
                glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_ColorAttachment);
                /// @todo option to modify samples count at runtime
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, spec.samplesCount, GL_RGBA8, spec.width, spec.height, GL_TRUE);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_ColorAttachment, 0);
            }
            if(spec.hasDepth)
            {
                glGenTextures(1, &m_DepthAttachment);
                glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_DepthAttachment);
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, spec.samplesCount, GL_DEPTH_COMPONENT24, spec.width, spec.height, GL_TRUE);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, m_DepthAttachment, 0);
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            glGenFramebuffers(1, &m_ResolveFBO);
            glBindFramebuffer(GL_FRAMEBUFFER, m_ResolveFBO);

            if(spec.hasColor)
            {
                glGenTextures(1, &m_ResolveColorAttachment);
                glBindTexture(GL_TEXTURE_2D, m_ResolveColorAttachment);
                GLTextureSpec colorSpecs = GLTextureSpec::FromTextureSpecifications(m_Specifications.colorSpecs);
                glTexImage2D(GL_TEXTURE_2D, 0, colorSpecs.internalFormat, spec.width, spec.height, 0, colorSpecs.format, colorSpecs.type, nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, colorSpecs.minFilter);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, colorSpecs.magFilter);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, colorSpecs.wrapModeS);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, colorSpecs.wrapModeT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, spec.colorSpecs.compareMode == TextureCompareMode::CompareRefToTexture ? GL_COMPARE_REF_TO_TEXTURE : GL_NONE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, colorSpecs.compareFunc);
                if(m_Specifications.colorSpecs.borderColor != COL_RGBA(-1.0f)){
                    float borderColor[] = {m_Specifications.colorSpecs.borderColor.r(), m_Specifications.colorSpecs.borderColor.g(), m_Specifications.colorSpecs.borderColor.b(), m_Specifications.colorSpecs.borderColor.a()};
                    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
                }
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ResolveColorAttachment, 0);
            }
            if(spec.hasDepth)
            {
                glGenTextures(1, &m_ResolveDepthAttachment);
                glBindTexture(GL_TEXTURE_2D, m_ResolveDepthAttachment);
                GLTextureSpec depthSpecs = GLTextureSpec::FromTextureSpecifications(m_Specifications.depthSpecs);
                glTexImage2D(GL_TEXTURE_2D, 0, depthSpecs.internalFormat, spec.width, spec.height, 0, depthSpecs.format, depthSpecs.type, nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, depthSpecs.minFilter);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, depthSpecs.magFilter);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, depthSpecs.wrapModeS);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, depthSpecs.wrapModeT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, spec.depthSpecs.compareMode == TextureCompareMode::CompareRefToTexture ? GL_COMPARE_REF_TO_TEXTURE : GL_NONE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, depthSpecs.compareFunc);
                if(m_Specifications.depthSpecs.borderColor != COL_RGBA(-1.0f)){
                    float borderColor[] = {m_Specifications.depthSpecs.borderColor.r(), m_Specifications.depthSpecs.borderColor.g(), m_Specifications.depthSpecs.borderColor.b(), m_Specifications.depthSpecs.borderColor.a()};
                    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
                }
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_ResolveDepthAttachment, 0);
            }
        }
        else {
            if (spec.hasColor)
            {
                glGenTextures(1, &m_ColorAttachment);
                glBindTexture(GL_TEXTURE_2D, m_ColorAttachment);
                GLTextureSpec colorSpecs = GLTextureSpec::FromTextureSpecifications(m_Specifications.colorSpecs);
                glTexImage2D(GL_TEXTURE_2D, 0, colorSpecs.internalFormat, spec.width, spec.height, 0, colorSpecs.format, colorSpecs.type, nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, colorSpecs.minFilter);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, colorSpecs.magFilter);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, colorSpecs.wrapModeS);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, colorSpecs.wrapModeT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, spec.colorSpecs.compareMode == TextureCompareMode::CompareRefToTexture ? GL_COMPARE_REF_TO_TEXTURE : GL_NONE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, colorSpecs.compareFunc);
                if(m_Specifications.colorSpecs.borderColor != COL_RGBA(-1.0f)){
                    float borderColor[] = {m_Specifications.colorSpecs.borderColor.r(), m_Specifications.colorSpecs.borderColor.g(), m_Specifications.colorSpecs.borderColor.b(), m_Specifications.colorSpecs.borderColor.a()};
                    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
                }
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachment, 0);
            }
            if(spec.hasDepth)
            {
                glGenTextures(1, &m_DepthAttachment);
                glBindTexture(GL_TEXTURE_2D, m_DepthAttachment);
                GLTextureSpec depthSpecs = GLTextureSpec::FromTextureSpecifications(m_Specifications.depthSpecs);
                glTexImage2D(GL_TEXTURE_2D, 0, depthSpecs.internalFormat, spec.width, spec.height, 0, depthSpecs.format, depthSpecs.type, nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, depthSpecs.minFilter);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, depthSpecs.magFilter);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, depthSpecs.wrapModeS);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, depthSpecs.wrapModeT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, spec.depthSpecs.compareMode == TextureCompareMode::CompareRefToTexture ? GL_COMPARE_REF_TO_TEXTURE : GL_NONE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, depthSpecs.compareFunc);
                if(m_Specifications.depthSpecs.borderColor != COL_RGBA(-1.0f)){
                    float borderColor[] = {m_Specifications.depthSpecs.borderColor.r(), m_Specifications.depthSpecs.borderColor.g(), m_Specifications.depthSpecs.borderColor.b(), m_Specifications.depthSpecs.borderColor.a()};
                    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
                }
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_DepthAttachment, 0);
            }
        }

        if (spec.hasColor)
        {
            GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
            glDrawBuffers(1, drawBuffers);
        }
        else
        {
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
        }

        CheckFBStatus();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (spec.multisampled)
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
        
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void GLFramebuffer::Destroy()
    {
        if (m_FBO) 
            glDeleteFramebuffers(1, &m_FBO);
        if(m_Specifications.hasColor) 
            glDeleteTextures(1, &m_ColorAttachment);
        if(m_Specifications.hasDepth)
            glDeleteTextures(1, &m_DepthAttachment);
        if (m_ResolveFBO) 
            glDeleteFramebuffers(1, &m_ResolveFBO);
        if (m_ResolveDepthAttachment) 
            glDeleteTextures(1, &m_ResolveDepthAttachment);
        if (m_ResolveColorAttachment) 
            glDeleteTextures(1, &m_ResolveColorAttachment);
    }

    void GLFramebuffer::Bind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    }

    void GLFramebuffer::Unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GLFramebuffer::CopyFrom(std::shared_ptr<Framebuffer> src)
    {
        auto srcSpecs = src->GetSpecifications();

        glBindFramebuffer(GL_READ_FRAMEBUFFER, src->GetHandle());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO);

        GLbitfield bitMask = 0;

        if(srcSpecs.hasColor && m_Specifications.hasColor){
            bitMask |= GL_COLOR_BUFFER_BIT;
        }
        if(srcSpecs.hasDepth && m_Specifications.hasDepth){
            bitMask |= GL_DEPTH_BUFFER_BIT;
        }

        if(bitMask == 0){
            return;
        }

        GLenum filter = GL_NEAREST;

        if(bitMask == GL_COLOR_BUFFER_BIT)
            filter = GL_LINEAR;

        glBlitFramebuffer(
            0, 0, srcSpecs.width, srcSpecs.height,
            0, 0, m_Specifications.width, m_Specifications.height,
            bitMask,
            filter
        );
    }

    void GLFramebuffer::BlitToScreen(uint32_t screenWidth, uint32_t screenHeight)
    {
        uint32_t readFBO = m_Specifications.multisampled ? m_ResolveFBO : m_FBO;

        glBindFramebuffer(GL_READ_FRAMEBUFFER, readFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        glBlitFramebuffer(
            0, 0, m_Specifications.width, m_Specifications.height,
            0, 0, screenWidth, screenHeight,
            GL_COLOR_BUFFER_BIT,
            GL_LINEAR
        );

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GLFramebuffer::ResolveMultisampled()
    {
        if (!m_Specifications.multisampled) return;

        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);   
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_ResolveFBO);
        glBlitFramebuffer(
            0, 0, m_Specifications.width, m_Specifications.height,
            0, 0, m_Specifications.width, m_Specifications.height,
            GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST
        );
        glBindFramebuffer(GL_FRAMEBUFFER, 0);    
    }

    void GLFramebuffer::AttachCubemapArray(uint32_t texture)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

        glFramebufferTexture(
            GL_FRAMEBUFFER,
            GL_DEPTH_ATTACHMENT,
            texture,
            0
        );

        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }

    void GLFramebuffer::DetachCubemapArray()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 0, 0);

        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    uint32_t GLFramebuffer::GetHandle() const
    {
        return m_FBO;
    }

    void GLFramebuffer::Resize(uint32_t width, uint32_t height)
    {
        m_Specifications.width = width;
        m_Specifications.height = height;

        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

        GLTextureSpec colorSpecs = GLTextureSpec::FromTextureSpecifications(m_Specifications.colorSpecs);
        GLTextureSpec depthSpecs = GLTextureSpec::FromTextureSpecifications(m_Specifications.depthSpecs);

        if (m_Specifications.multisampled) {
            
            if(m_Specifications.hasColor){
                glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_ColorAttachment);
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_Specifications.samplesCount, colorSpecs.internalFormat, width, height, GL_TRUE);

                glBindTexture(GL_TEXTURE_2D, m_ResolveColorAttachment);
                glTexImage2D(GL_TEXTURE_2D, 0, colorSpecs.internalFormat, width, height, 0, colorSpecs.format, colorSpecs.type, nullptr);
            }
            if(m_Specifications.hasDepth){
                glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_DepthAttachment);
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_Specifications.samplesCount, depthSpecs.internalFormat, width, height, GL_TRUE);

                glBindTexture(GL_TEXTURE_2D, m_ResolveDepthAttachment);
                glTexImage2D(GL_TEXTURE_2D, 0, depthSpecs.internalFormat, width, height, 0, depthSpecs.format, depthSpecs.type, nullptr);
            }
        }
        else {
            if (m_Specifications.hasColor)
            {
                glBindTexture(GL_TEXTURE_2D, m_ColorAttachment);
                glTexImage2D(GL_TEXTURE_2D, 0, colorSpecs.internalFormat, width, height, 0, colorSpecs.format, colorSpecs.type, nullptr);
            }
            if(m_Specifications.hasDepth)
            {
                glBindTexture(GL_TEXTURE_2D, m_DepthAttachment);
                glTexImage2D(GL_TEXTURE_2D, 0, depthSpecs.internalFormat, width, height, 0, depthSpecs.format, depthSpecs.type, nullptr);
            }
        }

        if (m_Specifications.hasColor)
        {
            GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
            glDrawBuffers(1, drawBuffers);
        }
        else
        {
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
        }

        CheckFBStatus();

        // Unbind everything
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (m_Specifications.multisampled)
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
        else
            glBindTexture(GL_TEXTURE_2D, 0);
    }

    bool GLFramebuffer::IsValid() const
    {
        return glIsFramebuffer(m_FBO);
    }

    uint32_t GLFramebuffer::GetColorAttachment() const
    {
        return m_ColorAttachment;
    }
    
    uint32_t GLFramebuffer::GetDepthAttachment() const
    {
        return m_DepthAttachment;
    }
    
    uint32_t GLFramebuffer::GetResolveColorAttachment() const
    {
        if (m_ResolveColorAttachment != 0)
            return m_ResolveColorAttachment;

        return m_ColorAttachment;
    }

    uint32_t GLFramebuffer::GetResolveDepthAttachment() const
    {
        if (m_ResolveDepthAttachment != 0)
            return m_ResolveDepthAttachment;

        return m_DepthAttachment;
    }
}