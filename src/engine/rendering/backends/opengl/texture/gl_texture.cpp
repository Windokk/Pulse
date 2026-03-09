#include "gl_texture.hpp"

#include "engine/rendering/backends/opengl/gl_utils.hpp"

namespace Pulse::Engine::Rendering{

    GLTexture::GLTexture(TextureSpecifications &spec, const void *data)
    {
        m_Specifications = spec;

        glGenTextures(1, &ID);
        glBindTexture(GL_TEXTURE_2D, ID);

        GLenum wrapModeS;
        GLenum wrapModeT;

        GLenum minFilter;
        GLenum magFilter;

        if(spec.magFilter == TextureFilter::Linear){
            magFilter = GL_LINEAR;
        }
        else{
            magFilter = GL_NEAREST;
        }

        if(spec.minFilter == TextureFilter::Linear){
            minFilter = GL_LINEAR;
        }
        else{
            minFilter = GL_NEAREST;
        }

        switch(spec.wrapS){
            case TextureWrap::Clamp:{
                wrapModeS = GL_CLAMP_TO_EDGE;
                break;
            }
            case TextureWrap::Mirror:{
                wrapModeS =  GL_MIRROR_CLAMP_TO_EDGE;
                break;
            }
            case TextureWrap::Repeat:{
                wrapModeS = GL_REPEAT;
                break;
            }
        }

        switch(spec.wrapT){
            case TextureWrap::Clamp:{
                wrapModeS = GL_CLAMP_TO_EDGE;
                break;
            }
            case TextureWrap::Mirror:{
                wrapModeT =  GL_MIRROR_CLAMP_TO_EDGE;
                break;
            }
            case TextureWrap::Repeat:{
                wrapModeT = GL_REPEAT;
                break;
            }
        }

        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapModeS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapModeT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

        GLenum internalFormat;

        switch(spec.format){
            case TextureFormat::R8:{
                internalFormat = GL_RED;
                break;
            }
            case TextureFormat::RG8:{
                internalFormat = GL_RG;
                break;
            }
            case TextureFormat::RGB8:{
                internalFormat = GL_RGB;
                break;
            }
            case TextureFormat::RGBA16F:{
                internalFormat = GL_RGBA;
                break;
            }
            default:{
                internalFormat = GL_RGB;
                break;
            }
        }

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, spec.width, spec.height, 0, internalFormat, GL_UNSIGNED_BYTE, data);

        if(spec.generateMips)
            glGenerateMipmap(GL_TEXTURE_2D);
    }

    void GLTexture::Bind(uint32_t slot) const
    {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, ID);
    }

    GLTexture::~GLTexture()
    {
        glDeleteTextures(1, &ID);
    }
}