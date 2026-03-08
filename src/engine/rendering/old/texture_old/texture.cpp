#include "texture.hpp"

#include "engine/filesystem/filesystem.hpp"
#include "engine/rendering/renderer/renderer.hpp"

#include "engine/core/engine.hpp"

#include <iostream>

namespace Pulse::Engine::Rendering{

    using namespace Filesystem;

    Image::Image(Filesystem::Path filepath)
    {
        unsigned char* data = nullptr;

        int width, height, nrChannels;

        if (filepath.Exists()) {
            std::string file = filepath.ReadFile();
            
            stbi_set_flip_vertically_on_load(true);
            data = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(file.data()),
                                            static_cast<int>(file.size()),
                                            &width, &height, &nrChannels, 0);

        } else {
            DEBUG_ERROR("Couldn't find texture : " + filepath.full);
            return;
        }

        if (data) {
            GLenum format;
            if (nrChannels == 1){
                format = GL_RED;
            }
            else if (nrChannels == 2){
                format = GL_RG;
            }
            else if (nrChannels == 3){
                format = GL_RGB;
            }
            else if (nrChannels == 4){
                format = GL_RGBA;
            }
            else{
                format = GL_RGB; // Default to RGB
            }

            texture = std::make_shared<Texture>(width, height, nrChannels, filterModes, wrapModes, format, format, data, true);

        } else {
            DEBUG_ERROR("Couldn't load texture : " + filepath.full);   
        }

        stbi_image_free(data);
        return;
    }

    Texture::Texture(int width, int height, int nrChannels, GLenum filterMode[2], GLenum wrapMode[2], GLenum internalFormat, GLenum format, unsigned char *data, bool generateMipmap)
    {
        infos.width = width;
        infos.height = height;
        infos.nrChannels = nrChannels;

        Core::GetEngine().GetGL()->GenTextures(1, &ID);
        Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_2D, ID);

        // Set texture parameters
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode[0]);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode[1]);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterMode[0]);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterMode[1]);

        Core::GetEngine().GetGL()->TexImage2D(GL_TEXTURE_2D, 0, internalFormat, infos.width, infos.height, 0, format, GL_UNSIGNED_BYTE, data);

        if(generateMipmap)
            Core::GetEngine().GetGL()->GenerateMipmap(GL_TEXTURE_2D);
    }

    void Texture::Bind(int unit)
    {
        Core::GetEngine().GetGL()->ActiveTexture(GL_TEXTURE0 + unit);
        Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_2D, ID);
    }

    void Texture::UnBind(int unit)
    {
        Core::GetEngine().GetGL()->ActiveTexture(GL_TEXTURE0 + unit);
        Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_2D, 0);
    }

    void Texture::Cleanup()
    {
        Core::GetEngine().GetGL()->DeleteTextures(1, &ID);
    }
}