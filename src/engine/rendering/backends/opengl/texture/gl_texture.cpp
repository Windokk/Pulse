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
        GLTextureSpec glSpecs;

        glSpecs.magFilter = (spec.magFilter == TextureFilter::Nearest) ? GL_NEAREST : GL_LINEAR;
        
        switch(spec.minFilter){
            case TextureFilter::Linear:{
                glSpecs.minFilter = GL_LINEAR;
                break;
            }
            case TextureFilter::LinearMipmapLinear:{
                glSpecs.minFilter = GL_LINEAR_MIPMAP_LINEAR;
                break;
            }
            case TextureFilter::LinearMipmapNearest:{
                glSpecs.minFilter = GL_LINEAR_MIPMAP_NEAREST;
                break;
            }
            case TextureFilter::Nearest:{
                glSpecs.minFilter = GL_NEAREST;
                break;
            }
            case TextureFilter::NearestMipmapNearest:{
                glSpecs.minFilter = GL_NEAREST_MIPMAP_NEAREST;
                break;
            }
            case TextureFilter::NearestMipmapLinear:{
                glSpecs.minFilter = GL_NEAREST_MIPMAP_LINEAR;
                break;
            }
        }

        switch(spec.wrapS){
            case TextureWrap::ClampEdge:{
                glSpecs.wrapModeS = GL_CLAMP_TO_EDGE;
                break;
            }
            case TextureWrap::ClampBorder:{
                glSpecs.wrapModeS = GL_CLAMP_TO_BORDER;
                break;
            }
            case TextureWrap::Mirror:{
                glSpecs.wrapModeS =  GL_MIRROR_CLAMP_TO_EDGE;
                break;
            }
            case TextureWrap::Repeat:{
                glSpecs.wrapModeS = GL_REPEAT;
                break;
            }
        }

        switch(spec.wrapT){
            case TextureWrap::ClampEdge:{
                glSpecs.wrapModeT = GL_CLAMP_TO_EDGE;
                break;
            }
            case TextureWrap::ClampBorder:{
                glSpecs.wrapModeT = GL_CLAMP_TO_BORDER;
                break;
            }
            case TextureWrap::Mirror:{
                glSpecs.wrapModeT =  GL_MIRROR_CLAMP_TO_EDGE;
                break;
            }
            case TextureWrap::Repeat:{
                glSpecs.wrapModeT = GL_REPEAT;
                break;
            }
        }

        switch(spec.wrapR){
            case TextureWrap::ClampEdge:{
                glSpecs.wrapModeR = GL_CLAMP_TO_EDGE;
                break;
            }
            case TextureWrap::ClampBorder:{
                glSpecs.wrapModeR = GL_CLAMP_TO_BORDER;
                break;
            }
            case TextureWrap::Mirror:{
                glSpecs.wrapModeR =  GL_MIRROR_CLAMP_TO_EDGE;
                break;
            }
            case TextureWrap::Repeat:{
                glSpecs.wrapModeR = GL_REPEAT;
                break;
            }
        }
      
        switch(spec.internalFormat){
            /// R
            case TextureInternalFormat::RED:{
                glSpecs.internalFormat = GL_RED;
                break;
            }
            case TextureInternalFormat::R16F:{
                glSpecs.internalFormat = GL_R16F;
                break;
            }
            /// RG
            case TextureInternalFormat::RG:{
                glSpecs.internalFormat = GL_RG;
                break;
            }
            case TextureInternalFormat::RG16F:{
                glSpecs.internalFormat = GL_RG16F;
                break;
            }
            /// RGB
            case TextureInternalFormat::RGB:{
                glSpecs.internalFormat = GL_RGB;
                break;
            }
            case TextureInternalFormat::RGB16F:{
                glSpecs.internalFormat = GL_RGB16F;
                break;
            }
            case TextureInternalFormat::RGB32I:{
                glSpecs.internalFormat = GL_RGB32I;
                break;
            }
            case TextureInternalFormat::RGB32F:{
                glSpecs.internalFormat = GL_RGB32F;
                break;
            }
            /// RGBA
            case TextureInternalFormat::RGBA8:{
                glSpecs.internalFormat = GL_RGBA8;
                break;
            }
            case TextureInternalFormat::RGBA:{
                glSpecs.internalFormat = GL_RGBA;
                break;
            }
            case TextureInternalFormat::RGBA32I:{
                glSpecs.internalFormat = GL_RGBA32I;
                break;
            }
            case TextureInternalFormat::RGBA32F:{
                glSpecs.internalFormat = GL_RGBA32F;
                break;
            }
            /// Depth
            case TextureInternalFormat::Depth16:{
                glSpecs.internalFormat = GL_DEPTH_COMPONENT16;
                break;
            }
            case TextureInternalFormat::Depth24Stencil8:{
                glSpecs.internalFormat = GL_DEPTH24_STENCIL8;
                break;
            }
            case TextureInternalFormat::Depth24:{
                glSpecs.internalFormat = GL_DEPTH_COMPONENT24;
                break;
            }
            case TextureInternalFormat::Depth32:{
                glSpecs.internalFormat = GL_DEPTH_COMPONENT32;
                break;
            }
            case TextureInternalFormat::Depth32F:{
                glSpecs.internalFormat = GL_DEPTH_COMPONENT32F;
                break;
            }
            default:{
                glSpecs.internalFormat = GL_RGB;
                break;
            }
        }

        switch(spec.format){
            case TextureFormat::RED:{
                glSpecs.format = GL_RED;
                break;
            }
            case TextureFormat::RG:{
                glSpecs.format = GL_RG;
                break;
            }
            case TextureFormat::RGB:{
                glSpecs.format = GL_RGB;
                break;
            }
            case TextureFormat::RGBA:{
                glSpecs.format = GL_RGBA;
                break;
            }
            case TextureFormat::Depth:{
                glSpecs.format = GL_DEPTH_COMPONENT;
                break;
            }
            default:{
                glSpecs.format = GL_RGB;
                break;
            }
        }

        switch(spec.compareFunc){
            case TextureCompareFunc::Always:      glSpecs.compareFunc = GL_ALWAYS; break;
            case TextureCompareFunc::Greater:     glSpecs.compareFunc = GL_GREATER; break;
            case TextureCompareFunc::Less:        glSpecs.compareFunc = GL_LESS; break;
            case TextureCompareFunc::LessOrEqual: glSpecs.compareFunc = GL_LEQUAL; break;
            case TextureCompareFunc::Never:       glSpecs.compareFunc = GL_NEVER; break;
            default:                              glSpecs.compareFunc = GL_LESS; break;
        }

        switch(spec.internalFormat)
        {
            case TextureInternalFormat::R16F:
            case TextureInternalFormat::RG16F:
            case TextureInternalFormat::RGB16F:
            case TextureInternalFormat::RGBAF:
            case TextureInternalFormat::RGBA32F:
            case TextureInternalFormat::RGB32F:
            case TextureInternalFormat::Depth32F:
                glSpecs.type = GL_FLOAT;
                break;

            case TextureInternalFormat::RGB32I:
            case TextureInternalFormat::RGBA32I:
                glSpecs.type = GL_INT;
                break;

            case TextureInternalFormat::Depth24Stencil8:
                glSpecs.type = GL_UNSIGNED_INT_24_8;
                break;

            case TextureInternalFormat::Depth16:
                glSpecs.type = GL_UNSIGNED_SHORT;
                break;

            case TextureInternalFormat::Depth24:
            case TextureInternalFormat::Depth32:
                glSpecs.type = GL_UNSIGNED_INT;
                break;

            default:
                glSpecs.type = GL_UNSIGNED_BYTE;
                break;
        }

        return glSpecs;
    }

}