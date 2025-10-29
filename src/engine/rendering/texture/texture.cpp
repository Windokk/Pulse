#include "texture.hpp"

#include "engine/filesystem/filesystem.hpp"
#include "engine/rendering/renderer/renderer.hpp"

#include "engine/core/engine.hpp"

#include <iostream>

namespace Pulse::Engine::Rendering{

    using namespace Filesystem;

    Texture::Texture(Filesystem::Path filepath)
    {
        Init(filepath);
    }

    void Texture::Init(Filesystem::Path filepath)
    {
        infos.filepath = std::make_shared<Filesystem::Path>(filepath);

        // Load and set up the texture
        Core::GetEngine().GetGL()->GenTextures(1, &ID);
        Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_2D, ID);

        // Set texture parameters
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        unsigned char* data = nullptr;

        if (infos.filepath->Exists()) {
            std::string file = infos.filepath->ReadFile();
            
            stbi_set_flip_vertically_on_load(true);
            data = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(file.data()),
                                            static_cast<int>(file.size()),
                                            &infos.width, &infos.height, &infos.nrChannels, 0);

        } else {
            DEBUG_ERROR("Couldn't find texture : " + infos.filepath->full);
            return;
        }

        if (data) {
            GLenum format;
            if (infos.nrChannels == 1){
                format = GL_RED;
            }
            else if (infos.nrChannels == 2){
                format = GL_RG;
            }
            else if (infos.nrChannels == 3){
                format = GL_RGB;
            }
            else if (infos.nrChannels == 4){
                format = GL_RGBA;
            }
            else{
                format = GL_RGB; // Default to RGB
            }

            Core::GetEngine().GetGL()->TexImage2D(GL_TEXTURE_2D, 0, format, infos.width, infos.height, 0, format, GL_UNSIGNED_BYTE, data);
            Core::GetEngine().GetGL()->GenerateMipmap(GL_TEXTURE_2D);
        } else {
            DEBUG_ERROR("Couldn't load texture : " + filepath.full);   
        }

        stbi_image_free(data);
        return;
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

        Core::GetEngine().GetGL()->DeleteVertexArrays(1, &VAO);
        Core::GetEngine().GetGL()->DeleteBuffers(1, &VBO);
        Core::GetEngine().GetGL()->DeleteBuffers(1, &EBO);
        VAO = VBO = EBO = 0;
    }
}