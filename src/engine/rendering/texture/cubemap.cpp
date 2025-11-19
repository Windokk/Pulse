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

        Init(filepath);
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

    void Cubemap::Init(Filesystem::Path filepath)
    {
        GenerateMesh();

        infos.filepath = std::make_shared<Filesystem::Path>(filepath);

        if(infos.filepath->IsDirectory()){
            CreateFromFolder();
        }
        else{
            CreateFromHDR();
        }
        
        return;
    }

    void Cubemap::CreateFromFolder(){
         
        // Load and set up the texture
        Core::GetEngine().GetGL()->GenTextures(1, &ID);
        Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_CUBE_MAP, ID);

        unsigned char* data = nullptr;

        std::vector<Filesystem::FileInfo> files = Core::GetEngine().GetFileManager()->ListDirectory(*infos.filepath.get(), {Filesystem::Type::T_IMAGE}, false, false);
        
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
            DEBUG_ERROR("Couldn't find textures (or too much files in the folder) : " + infos.filepath->full);
            return;
        }

        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }
    
    void Cubemap::CreateFromHDR(){

        std::string name = Core::GetEngine().GetFileManager()->GetFileInfos(*infos.filepath.get()).nameInProject;

        std::shared_ptr<Texture> hdrTex = Core::GetEngine().GetResourcesManager()->GetTexture(name);

        std::shared_ptr<Shader> equirectangularToCubemapShader = Core::GetEngine().GetResourcesManager()->GetShader("shaders\\ibl\\equirectToCubemap");

        if(!hdrTex){
            DEBUG_ERROR("HDR cubemap couldn't be created from texture : ", infos.filepath->full, " because the texture is not loaded !");
            return;
        }

        if(!equirectangularToCubemapShader)
        {
            DEBUG_ERROR("HDR cubemap couldn't be create because shader \"shaders\\ibl\\equirectToCubemap\" is not loaded !");
            return;
        }

        const unsigned int faceRes = 512;
        infos.width = infos.height = faceRes;
        infos.nrChannels = 3;

        Core::GetEngine().GetGL()->GenTextures(1, &ID);
        Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_CUBE_MAP, ID);

        for (unsigned int i = 0; i < 6; ++i) {
            Core::GetEngine().GetGL()->TexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 
                 infos.width, infos.height, 0, GL_RGB, GL_FLOAT, nullptr);
        }

        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        GLuint captureFBO, captureRBO;
        Core::GetEngine().GetGL()->GenFramebuffers(1, &captureFBO);
        Core::GetEngine().GetGL()->GenRenderbuffers(1, &captureRBO);

        Core::GetEngine().GetGL()->BindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        Core::GetEngine().GetGL()->BindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        Core::GetEngine().GetGL()->RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, infos.width, infos.height);
        Core::GetEngine().GetGL()->FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 captureViews[] = 
        {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };

        equirectangularToCubemapShader->Activate();
        equirectangularToCubemapShader->setInt("equirectangularMap", 0);
        equirectangularToCubemapShader->setMat4("projection", captureProjection);
        Core::GetEngine().GetGL()->ActiveTexture(GL_TEXTURE0);
        Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_2D, hdrTex->GetID());

        Core::GetEngine().GetGL()->Viewport(0, 0, infos.width, infos.height); // Viewport => capture's dimensions
        Core::GetEngine().GetGL()->BindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        for (unsigned int i = 0; i < 6; ++i)
        {
            equirectangularToCubemapShader->setMat4("view", captureViews[i]);
            Core::GetEngine().GetGL()->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, ID, 0);
            Core::GetEngine().GetGL()->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            RenderUnitCube(); // renders a 1x1 cube
        }
        Core::GetEngine().GetGL()->BindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Cubemap::RenderUnitCube()
    {
        Core::GetEngine().GetGL()->BindVertexArray(VAO);
        Core::GetEngine().GetGL()->DrawArrays(GL_TRIANGLES, 0, 36);
        Core::GetEngine().GetGL()->BindVertexArray(0);
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