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
        Core::GetEngine().GetGL()->BindVertexArray(cubeVAO);
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
        GenerateGeometry();

        infos.filepath = std::make_shared<Filesystem::Path>(filepath);

        if(infos.filepath->IsDirectory()){
            CreateFromFolder();
        }
        else{
            CreateFromHDR();
        }
        
        if(cubemapID != 0){
            CreateIrradiance();
            CreatePrefilter();
            CreateBRDFLUT();
        }

        return;
    }

    void Cubemap::CreateFromFolder(){
         
        // Load and set up the texture
        Core::GetEngine().GetGL()->GenTextures(1, &cubemapID);
        Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_CUBE_MAP, cubemapID);

        unsigned char* data = nullptr;

        std::vector<Filesystem::FileInfos> files = Core::GetEngine().GetFileManager()->ListDirectory(*infos.filepath.get(), {Filesystem::Type::T_IMAGE}, false, false);
        
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
            DEBUG_ERROR("HDR cubemap couldn't be created because shader \"shaders\\ibl\\equirectToCubemap\" is not loaded !");
            return;
        }

        const unsigned int faceRes = 512;
        infos.width = infos.height = faceRes;
        infos.nrChannels = 3;

        Core::GetEngine().GetGL()->GenTextures(1, &cubemapID);
        Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_CUBE_MAP, cubemapID);

        for (unsigned int i = 0; i < 6; ++i) {
            Core::GetEngine().GetGL()->TexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 
                 infos.width, infos.height, 0, GL_RGB, GL_FLOAT, nullptr);
        }

        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Core::GetEngine().GetGL()->GenFramebuffers(1, &captureFBO);
        Core::GetEngine().GetGL()->GenRenderbuffers(1, &captureRBO);

        Core::GetEngine().GetGL()->BindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        Core::GetEngine().GetGL()->BindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        Core::GetEngine().GetGL()->RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, infos.width, infos.height);
        Core::GetEngine().GetGL()->FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

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
                                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemapID, 0);
            Core::GetEngine().GetGL()->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            RenderUnitCube(); // renders a 1x1 cube
        }
        Core::GetEngine().GetGL()->BindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Cubemap::CreateIrradiance(){

        std::shared_ptr<Shader> irradianceShader = Core::GetEngine().GetResourcesManager()->GetShader("shaders\\ibl\\irradiance");

        if(!irradianceShader)
        {
            DEBUG_ERROR("Cubemap irradiance map couldn't be created because shader \"shaders\\ibl\\irradiance\" is not loaded !");
            return;
        }

        Core::GetEngine().GetGL()->GenTextures(1, &irradianceMapID);
        Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_CUBE_MAP, irradianceMapID);
        for (unsigned int i = 0; i < 6; ++i)
        {
            Core::GetEngine().GetGL()->TexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Core::GetEngine().GetGL()->BindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        Core::GetEngine().GetGL()->BindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        Core::GetEngine().GetGL()->RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

        irradianceShader->Activate();
        irradianceShader->setInt("environmentMap", 0);
        irradianceShader->setMat4("projection", captureProjection);
        Core::GetEngine().GetGL()->ActiveTexture(GL_TEXTURE0);
        Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_CUBE_MAP, cubemapID);

        Core::GetEngine().GetGL()->Viewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions.
        Core::GetEngine().GetGL()->BindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        for (unsigned int i = 0; i < 6; ++i)
        {
            irradianceShader->setMat4("view", captureViews[i]);
            Core::GetEngine().GetGL()->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMapID, 0);
            Core::GetEngine().GetGL()->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            RenderUnitCube();
        }
        Core::GetEngine().GetGL()->BindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Cubemap::CreatePrefilter(){

        std::shared_ptr<Shader> prefilterShader = Core::GetEngine().GetResourcesManager()->GetShader("shaders\\ibl\\prefilter");

        if(!prefilterShader)
        {
            DEBUG_ERROR("Cubemap prefilter map couldn't be created because shader \"shaders\\ibl\\equirectToCubemap\" is not loaded !");
            return;
        }

        Core::GetEngine().GetGL()->GenTextures(1, &prefilterMapID);
        Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_CUBE_MAP, prefilterMapID);
        for (unsigned int i = 0; i < 6; ++i)
        {
            Core::GetEngine().GetGL()->TexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // be sure to set minification filter to mip_linear 
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // generate mipmaps for the cubemap so OpenGL automatically allocates the required memory.
        Core::GetEngine().GetGL()->GenerateMipmap(GL_TEXTURE_CUBE_MAP);

        prefilterShader->Activate();
        prefilterShader->setInt("environmentMap", 0);
        prefilterShader->setMat4("projection", captureProjection);
        Core::GetEngine().GetGL()->ActiveTexture(GL_TEXTURE0);
        Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_CUBE_MAP, cubemapID);

        Core::GetEngine().GetGL()->BindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        unsigned int maxMipLevels = 5;
        for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
        {
            // reisze framebuffer according to mip-level size.
            unsigned int mipWidth  = static_cast<unsigned int>(128 * std::pow(0.5, mip));
            unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));
            Core::GetEngine().GetGL()->BindRenderbuffer(GL_RENDERBUFFER, captureRBO);
            Core::GetEngine().GetGL()->RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
            Core::GetEngine().GetGL()->Viewport(0, 0, mipWidth, mipHeight);

            float roughness = (float)mip / (float)(maxMipLevels - 1);
            prefilterShader->setFloat("roughness", roughness);
            for (unsigned int i = 0; i < 6; ++i)
            {
                prefilterShader->setMat4("view", captureViews[i]);
                Core::GetEngine().GetGL()->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMapID, mip);

                Core::GetEngine().GetGL()->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                RenderUnitCube();
            }
        }
        Core::GetEngine().GetGL()->BindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Cubemap::CreateBRDFLUT(){
        
        std::shared_ptr<Shader> brdfShader = Core::GetEngine().GetResourcesManager()->GetShader("shaders\\ibl\\brdf");

        if(!brdfShader)
        {
            DEBUG_ERROR("Cubemap brdf lookup table couldn't be created because shader \"shaders\\ibl\\brdf\" is not loaded !");
            return;
        }

        Core::GetEngine().GetGL()->GenTextures(1, &brdfLUTTextureID);

        // pre-allocate enough memory for the LUT texture.
        Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_2D, brdfLUTTextureID);
        Core::GetEngine().GetGL()->TexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
        // be sure to set wrapping mode to GL_CLAMP_TO_EDGE
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
        Core::GetEngine().GetGL()->BindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        Core::GetEngine().GetGL()->BindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        Core::GetEngine().GetGL()->RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
        Core::GetEngine().GetGL()->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTextureID, 0);

        Core::GetEngine().GetGL()->Viewport(0, 0, 512, 512);
        brdfShader->Activate();
        Core::GetEngine().GetGL()->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        RenderUnitQuad();

        Core::GetEngine().GetGL()->BindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Cubemap::RenderUnitCube()
    {
        Core::GetEngine().GetGL()->BindVertexArray(cubeVAO);
        Core::GetEngine().GetGL()->DrawArrays(GL_TRIANGLES, 0, 36);
        Core::GetEngine().GetGL()->BindVertexArray(0);
    }

    void Cubemap::RenderUnitQuad(){
        Core::GetEngine().GetGL()->BindVertexArray(quadVAO);
        Core::GetEngine().GetGL()->DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        Core::GetEngine().GetGL()->BindVertexArray(0);
    }

    void Cubemap::Bind()
    {
        Core::GetEngine().GetGL()->ActiveTexture(GL_TEXTURE0);
        Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_CUBE_MAP, cubemapID);
    }

    void Cubemap::UnBind()
    {
        Core::GetEngine().GetGL()->ActiveTexture(GL_TEXTURE0);
        Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }

    void Cubemap::Cleanup()
    {
        Core::GetEngine().GetGL()->DeleteTextures(1, &cubemapID);

        Core::GetEngine().GetGL()->DeleteVertexArrays(1, &cubeVAO);
        Core::GetEngine().GetGL()->DeleteBuffers(1, &cubeVBO);
        cubeVAO = cubeVBO = 0;
    }
    
    void Cubemap::GenerateGeometry()
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

        gl.GenVertexArrays(1, &cubeVAO);
        gl.GenBuffers(1, &cubeVBO);
        gl.BindVertexArray(cubeVAO);
        gl.BindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        gl.BufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
        gl.EnableVertexAttribArray(0);
        gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

        gl.BindVertexArray(0);

        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        gl.GenVertexArrays(1, &quadVAO);
        gl.GenBuffers(1, &quadVBO);
        gl.BindVertexArray(quadVAO);
        gl.BindBuffer(GL_ARRAY_BUFFER, quadVBO);
        gl.BufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        gl.EnableVertexAttribArray(0);
        gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        gl.EnableVertexAttribArray(1);
        gl.VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }

}