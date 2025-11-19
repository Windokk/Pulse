#include "cubemap.hpp"

#include "engine/filesystem/filesystem.hpp"
#include "engine/rendering/renderer/renderer.hpp"

#include "engine/core/engine.hpp"

#include <iostream>

namespace Pulse::Engine::Rendering{

    using namespace Filesystem;

    Cubemap::Cubemap(Filesystem::Path filepath)
    {
        if(!filepath.Exists() || filepath.GetParent().empty()){
            DEBUG_ERROR("Texture at path : ", filepath.full, "doesn't exist, or doesn't have a valid parent");
            return;
        }

        Init(filepath.GetParent());
    }

    void Cubemap::Draw(std::shared_ptr<Shader> shader, glm::mat4 view, glm::mat4 projection)
    {
        Core::GetEngine().GetGL()->DepthFunc(GL_LEQUAL);
        shader->Activate();
        Core::GetEngine().GetGL()->BindVertexArray(VAO);
        Bind();
        view = glm::mat4(glm::mat3(view));
        shader->setMat4("view", view);
        shader->setMat4("projection", projection);
        shader->setInt("gCubemapTexture", 0);
        Core::GetEngine().GetGL()->DrawArrays(GL_TRIANGLES, 0, 36);
        shader->Deactivate();
        UnBind();
        Core::GetEngine().GetGL()->DepthFunc(GL_LESS);
    }

    void Cubemap::Init(Filesystem::Path folder)
    {
        GenerateMesh();

        infos.folder = std::make_shared<Filesystem::Path>(folder);

        // Load and set up the texture
        Core::GetEngine().GetGL()->GenTextures(1, &ID);
        Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_CUBE_MAP, ID);

        unsigned char* data = nullptr;

        if(!infos.folder->Exists()){
            DEBUG_ERROR("Couldn't find textures folder : " + infos.folder->full);
            return;
        }

        std::vector<Filesystem::FileInfo> files = Core::GetEngine().GetFileManager()->ListDirectory(*infos.folder.get(), {Filesystem::Type::T_IMAGE}, false, false);
        
        stbi_set_flip_vertically_on_load(false);

        if (files.size() == 6) {
            
            for(int i = 0; i < files.size(); i++){

                std::string file = files[i].path.ReadFile();
                data = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(file.data()),
                                                static_cast<int>(file.size()),
                                                &infos.width, &infos.height, &infos.nrChannels, 0);
            
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

                    Core::GetEngine().GetGL()->TexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, infos.width, infos.height, 0, format, GL_UNSIGNED_BYTE, data);
                    
                }
                else {
                    DEBUG_ERROR("Couldn't load texture : " + files[i].path.full);   
                }

                stbi_image_free(data);

            }


        } else {
            DEBUG_ERROR("Couldn't find textures (or too much files in the folder) : " + infos.folder->full);
            return;
        }

        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        
        return;
    }

    void Cubemap::Bind()
    {
        Core::GetEngine().GetGL()->ActiveTexture(GL_TEXTURE0);
        Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_CUBE_MAP, ID);
    }

    void Cubemap::UnBind()
    {
        Core::GetEngine().GetGL()->ActiveTexture(GL_TEXTURE0);
        Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }

    void Cubemap::Cleanup()
    {
        Core::GetEngine().GetGL()->DeleteTextures(1, &ID);

        Core::GetEngine().GetGL()->DeleteVertexArrays(1, &VAO);
        Core::GetEngine().GetGL()->DeleteBuffers(1, &VBO);
        Core::GetEngine().GetGL()->DeleteBuffers(1, &EBO);
        VAO = VBO = EBO = 0;
    }

    
    void Cubemap::GenerateMesh()
    {
        float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

        auto& gl = *Core::GetEngine().GetGL();

        gl.GenVertexArrays(1, &VAO);
        gl.GenBuffers(1, &VBO);
        gl.BindVertexArray(VAO);
        gl.BindBuffer(GL_ARRAY_BUFFER, VBO);
        gl.BufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
        gl.EnableVertexAttribArray(0);
        gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

        gl.BindVertexArray(0);
    }

}