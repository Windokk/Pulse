#include "gl_texture.hpp"

#include "engine/rendering/backends/opengl/gl_utils.hpp"

namespace Pulse::Engine::Rendering{

    GLTexture2D::GLTexture2D(TextureSpecifications &specs, const void *data)
    {
        m_Specifications = specs;

        glGenTextures(1, &ID);
        glBindTexture(GL_TEXTURE_2D, ID);

        GLTextureSpec glSpecs = GLTextureSpec::FromTextureSpecifications(specs);

        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glSpecs.wrapModeS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glSpecs.wrapModeT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glSpecs.minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glSpecs.magFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, specs.compareMode == TextureCompareMode::CompareRefToTexture ? GL_COMPARE_REF_TO_TEXTURE : GL_NONE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, glSpecs.compareFunc);
        if(specs.borderColor != COL_RGBA(-1.0f)){
            float borderColor[] = {specs.borderColor.r(), specs.borderColor.g(), specs.borderColor.b(), specs.borderColor.a()};
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        }

        glTexImage2D(GL_TEXTURE_2D, 0, glSpecs.internalFormat, specs.width, specs.height, 0, glSpecs.format, glSpecs.type, data);

        if(specs.generateMips)
            glGenerateMipmap(GL_TEXTURE_2D);
    }

    void GLTexture2D::Bind(uint32_t slot) const
    {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, ID);
    }

    bool GLTexture2D::IsValid() const
    {
        return glIsTexture(ID);
    }

    GLTexture2D::~GLTexture2D()
    {
        glDeleteTextures(1, &ID);
    }

    GLTextureSpec GLTextureSpec::FromTextureSpecifications(const TextureSpecifications &spec)
    {
        GLTextureSpec glSpecs{};

        // Filtering
        glSpecs.magFilter = (spec.magFilter == TextureFilter::Nearest) ? GL_NEAREST : GL_LINEAR;

        switch (spec.minFilter)
        {
            case TextureFilter::Linear:                 glSpecs.minFilter = GL_LINEAR; break;
            case TextureFilter::LinearMipmapLinear:     glSpecs.minFilter = GL_LINEAR_MIPMAP_LINEAR; break;
            case TextureFilter::LinearMipmapNearest:    glSpecs.minFilter = GL_LINEAR_MIPMAP_NEAREST; break;
            case TextureFilter::Nearest:                glSpecs.minFilter = GL_NEAREST; break;
            case TextureFilter::NearestMipmapNearest:   glSpecs.minFilter = GL_NEAREST_MIPMAP_NEAREST; break;
            case TextureFilter::NearestMipmapLinear:    glSpecs.minFilter = GL_NEAREST_MIPMAP_LINEAR; break;
            default:                                    glSpecs.minFilter = GL_LINEAR; break;
        }

        // Wrapping
        auto WrapToGL = [](TextureWrap wrap)
        {
            switch (wrap)
            {
                case TextureWrap::ClampEdge:   return GL_CLAMP_TO_EDGE;
                case TextureWrap::ClampBorder: return GL_CLAMP_TO_BORDER;
                case TextureWrap::Mirror:      return GL_MIRRORED_REPEAT; // fixed
                case TextureWrap::Repeat:      return GL_REPEAT;
            }
            return GL_REPEAT;
        };

        glSpecs.wrapModeS = WrapToGL(spec.wrapS);
        glSpecs.wrapModeT = WrapToGL(spec.wrapT);
        glSpecs.wrapModeR = WrapToGL(spec.wrapR);

        // Internal Format + Format + Type
        switch (spec.internalFormat)
        {
            // R 
            case TextureInternalFormat::RED:
                glSpecs.internalFormat = GL_R8;
                glSpecs.format = GL_RED;
                glSpecs.type = GL_UNSIGNED_BYTE;
                break;

            case TextureInternalFormat::R16F:
                glSpecs.internalFormat = GL_R16F;
                glSpecs.format = GL_RED;
                glSpecs.type = GL_FLOAT;
                break;

            // RG
            case TextureInternalFormat::RG:
                glSpecs.internalFormat = GL_RG8;
                glSpecs.format = GL_RG;
                glSpecs.type = GL_UNSIGNED_BYTE;
                break;

            case TextureInternalFormat::RG16F:
                glSpecs.internalFormat = GL_RG16F;
                glSpecs.format = GL_RG;
                glSpecs.type = GL_FLOAT;
                break;

            // RGB
            case TextureInternalFormat::RGB:
            case TextureInternalFormat::RGB8:
                glSpecs.internalFormat = GL_RGB8;
                glSpecs.format = GL_RGB;
                glSpecs.type = GL_UNSIGNED_BYTE;
                break;

            case TextureInternalFormat::RGB16F:
                glSpecs.internalFormat = GL_RGB16F;
                glSpecs.format = GL_RGB;
                glSpecs.type = GL_FLOAT;
                break;

            case TextureInternalFormat::RGB32F:
                glSpecs.internalFormat = GL_RGB32F;
                glSpecs.format = GL_RGB;
                glSpecs.type = GL_FLOAT;
                break;

            case TextureInternalFormat::RGB32I:
                glSpecs.internalFormat = GL_RGB32I;
                glSpecs.format = GL_RGB_INTEGER;
                glSpecs.type = GL_INT;
                break;

            // RGBA
            case TextureInternalFormat::RGBA:
            case TextureInternalFormat::RGBA8:
                glSpecs.internalFormat = GL_RGBA8;
                glSpecs.format = GL_RGBA;
                glSpecs.type = GL_UNSIGNED_BYTE;
                break;

            case TextureInternalFormat::RGBA16F:
                glSpecs.internalFormat = GL_RGBA16F;
                glSpecs.format = GL_RGBA;
                glSpecs.type = GL_FLOAT;
                break;

            case TextureInternalFormat::RGBA32F:
                glSpecs.internalFormat = GL_RGBA32F;
                glSpecs.format = GL_RGBA;
                glSpecs.type = GL_FLOAT;
                break;

            case TextureInternalFormat::RGBA32I:
                glSpecs.internalFormat = GL_RGBA32I;
                glSpecs.format = GL_RGBA_INTEGER;
                glSpecs.type = GL_INT;
                break;

            // Depth
            case TextureInternalFormat::Depth16:
                glSpecs.internalFormat = GL_DEPTH_COMPONENT16;
                glSpecs.format = GL_DEPTH_COMPONENT;
                glSpecs.type = GL_UNSIGNED_SHORT;
                break;

            case TextureInternalFormat::Depth24:
                glSpecs.internalFormat = GL_DEPTH_COMPONENT24;
                glSpecs.format = GL_DEPTH_COMPONENT;
                glSpecs.type = GL_UNSIGNED_INT;
                break;

            case TextureInternalFormat::Depth32:
                glSpecs.internalFormat = GL_DEPTH_COMPONENT32;
                glSpecs.format = GL_DEPTH_COMPONENT;
                glSpecs.type = GL_UNSIGNED_INT;
                break;

            case TextureInternalFormat::Depth32F:
                glSpecs.internalFormat = GL_DEPTH_COMPONENT32F;
                glSpecs.format = GL_DEPTH_COMPONENT;
                glSpecs.type = GL_FLOAT;
                break;

            case TextureInternalFormat::Depth24Stencil8:
                glSpecs.internalFormat = GL_DEPTH24_STENCIL8;
                glSpecs.format = GL_DEPTH_STENCIL;
                glSpecs.type = GL_UNSIGNED_INT_24_8;
                break;

            default:
                glSpecs.internalFormat = GL_RGBA8;
                glSpecs.format = GL_RGBA;
                glSpecs.type = GL_UNSIGNED_BYTE;
                break;
        }

        // Compare func (for shadow maps)
        switch (spec.compareFunc)
        {
            case TextureCompareFunc::Always:      glSpecs.compareFunc = GL_ALWAYS; break;
            case TextureCompareFunc::Greater:     glSpecs.compareFunc = GL_GREATER; break;
            case TextureCompareFunc::Less:        glSpecs.compareFunc = GL_LESS; break;
            case TextureCompareFunc::LessOrEqual: glSpecs.compareFunc = GL_LEQUAL; break;
            case TextureCompareFunc::Never:       glSpecs.compareFunc = GL_NEVER; break;
            default:                             glSpecs.compareFunc = GL_LESS; break;
        }

        return glSpecs;
    }

}