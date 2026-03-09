#include "gl_cubemap.hpp"

#include "engine/rendering/backends/opengl/gl_utils.hpp"

namespace Pulse::Engine::Rendering{
    
    GLCubemap::GLCubemap(const TextureSpecifications &spec, std::array<unsigned char*, 6> faces)
    {
        m_Specifications = spec;

        glGenTextures(1, &ID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, ID);

        GLenum wrapModeS;
        GLenum wrapModeT;
        GLenum wrapModeR;

        GLenum minFilter;
        GLenum magFilter;

        GLenum internalFormat;

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

        switch(spec.wrapR){
            case TextureWrap::Clamp:{
                wrapModeR = GL_CLAMP_TO_EDGE;
                break;
            }
            case TextureWrap::Mirror:{
                wrapModeR =  GL_MIRROR_CLAMP_TO_EDGE;
                break;
            }
            case TextureWrap::Repeat:{
                wrapModeR = GL_REPEAT;
                break;
            }
        }
      
        switch(spec.format){
            case TextureFormat::RED:{
                internalFormat = GL_RED;
                break;
            }
            case TextureFormat::RG:{
                internalFormat = GL_RG;
                break;
            }
            case TextureFormat::RGB:{
                internalFormat = GL_RGB;
                break;
            }
            case TextureFormat::RGBA:{
                internalFormat = GL_RGBA;
                break;
            }
            default:{
                internalFormat = GL_RGB;
                break;
            }
        }

        for(int i = 0; i < 6; i++){
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, spec.width, spec.height, 0, internalFormat, GL_UNSIGNED_BYTE, faces[i]);
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, minFilter);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, magFilter);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, wrapModeS);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, wrapModeT);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, wrapModeR);

        if(spec.generateMips)
        {
            glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
        }
    }

    void GLCubemap::Bind(uint32_t slot) const
    {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_CUBE_MAP, ID);
    }
    
    GLCubemap::~GLCubemap()
    {
        glDeleteTextures(1, &ID);
    }
}