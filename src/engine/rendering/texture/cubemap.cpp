#include "cubemap.hpp"

#include "engine/filesystem/filesystem.hpp"
#include "engine/rendering/renderer/renderer.hpp"

#include "engine/core/engine.hpp"

#include <iostream>

namespace Pulse::Engine::Rendering{

    using namespace Filesystem;


    Cubemap::Cubemap(int width, int height, GLenum filterMode[2], GLenum wrapMode[3], GLenum internalFormat, GLenum format, unsigned char* data[6], bool generateMipmap)
    {
        Core::GetEngine().GetGL()->GenTextures(1, &ID);
        Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_CUBE_MAP, ID);

        for(int i = 0; i < 6; i++){
            Core::GetEngine().GetGL()->TexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data[i]);
        }

        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, filterMode[0]);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, filterMode[1]);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, wrapMode[0]);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, wrapMode[1]);
        Core::GetEngine().GetGL()->TexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, wrapMode[2]);

        if(generateMipmap)
        {
            Core::GetEngine().GetGL()->GenerateMipmap(GL_TEXTURE_CUBE_MAP);
        }
    }

    Cubemap::Cubemap(unsigned int ID)
    {
        this->ID = ID;
    }

    void Cubemap::Draw(int cubeVAO, std::shared_ptr<Shader> shader, glm::mat4 view, glm::mat4 projection)
    {
        Core::GetEngine().GetGL()->DepthFunc(GL_LEQUAL);
        shader->Activate();
        Core::GetEngine().GetGL()->BindVertexArray(cubeVAO);
        Bind(0);
        view = glm::mat4(glm::mat3(view));
        shader->SetMat4("view", view);
        shader->SetMat4("projection", projection);
        shader->SetInt("gCubemapTexture", 0);
        Core::GetEngine().GetGL()->DrawArrays(GL_TRIANGLES, 0, 36);
        shader->Deactivate();
        UnBind(0);
        Core::GetEngine().GetGL()->DepthFunc(GL_LESS);
    }

    EnvironmentMap::EnvironmentMap(Filesystem::Path filepath)
    {
        if(!filepath.Exists() || filepath.GetParent().empty()){
            DEBUG_ERROR("Texture at path : ", filepath.full, "doesn't exist, or doesn't have a valid parent");
            return;
        }

        Init(filepath);
    }

    void EnvironmentMap::Init(Filesystem::Path filepath)
    {
        GenerateGeometry();

        infos.filepath = std::make_shared<Filesystem::Path>(filepath);

        if(infos.filepath->IsDirectory()){
            CreateFromFolder();
        }
        else{
            CreateFromHDR();
        }
        
        if(cubemap->GetID() != 0){
            CreateIrradiance();
            CreatePrefilter();
            CreateBRDFLUT();
        }

        return;
    }

    void EnvironmentMap::Draw(std::shared_ptr<Shader> shader, glm::mat4 view, glm::mat4 projection)
    {
        cubemap->Draw(cubeVAO, shader, view, projection);
    }

    void EnvironmentMap::Cleanup()
    {
        Core::GetEngine().GetGL()->DeleteVertexArrays(1, &cubeVAO);
        Core::GetEngine().GetGL()->DeleteBuffers(1, &cubeVBO);
        Core::GetEngine().GetGL()->DeleteFramebuffers(1, &captureFBO);
        Core::GetEngine().GetGL()->DeleteRenderbuffers(1, &captureRBO);
        captureFBO = captureRBO = 0;
        cubeVAO = cubeVBO = 0;
    }

    void EnvironmentMap::CreateFromFolder()
    {

        unsigned char* data[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};

        std::vector<Filesystem::FileInfos> files = Core::GetEngine().GetFileManager()->ListDirectory(*infos.filepath.get(), {Filesystem::Type::T_IMAGE}, false, false);
        
        stbi_set_flip_vertically_on_load(false);

        if (files.size() == 6) {
            
            GLenum format;

            for(int i = 0; i < files.size(); i++){

                std::string file = files[i].path.ReadFile();
                data[i] = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(file.data()),
                                                static_cast<int>(file.size()),
                                                &infos.width, &infos.height, &infos.nrChannels, 0);

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

                if(!data[i]){
                    DEBUG_ERROR("Couldn't load texture : " + files[i].path.full);   
                }
            }
            
            for (int i = 0; i < 6; i++) {
                stbi_image_free(data[i]);
            }
            
            GLenum filters[2] = {GL_LINEAR, GL_LINEAR};
            GLenum wrapModes[3] = {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE};

            cubemap = std::make_shared<Cubemap>(infos.width, infos.height, filters, wrapModes, format, format, data, false);

        } else {
            DEBUG_ERROR("Couldn't find textures (or too much files in the folder) while creating cubemap : " + infos.filepath->full);
            return;
        }
    }

    void EnvironmentMap::CreateFromHDR(){

        std::string name = Core::GetEngine().GetFileManager()->GetFileInfos(*infos.filepath.get()).nameInProject;

        std::shared_ptr<Pulse::Engine::Rendering::Image> hdrImg = Core::GetEngine().GetResourcesManager()->GetImage(name);

        if(!hdrImg){
            DEBUG_ERROR("HDR cubemap couldn't be created from texture : ", infos.filepath->full, " because the texture is not loaded !");
            return;
        }

        std::shared_ptr<Texture> hdrTex = hdrImg->texture;

        std::shared_ptr<Shader> equirectangularToCubemapShader = Core::GetEngine().GetResourcesManager()->GetShader("shaders/ibl/equirectToCubemap");

        if(!equirectangularToCubemapShader)
        {
            DEBUG_ERROR("HDR cubemap couldn't be created because shader \"shaders/ibl/equirectToCubemap\" is not loaded !");
            return;
        }

        infos.width = infos.height = 512;
        infos.nrChannels = 3;

        unsigned char* data[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
        GLenum filters[2] = {GL_LINEAR, GL_LINEAR};
        GLenum wrapModes[3] = {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE};

        cubemap = std::make_shared<Cubemap>(infos.width, infos.height, filters, wrapModes, GL_RGB16F, GL_RGB, data, false);

        Core::GetEngine().GetGL()->GenFramebuffers(1, &captureFBO);
        Core::GetEngine().GetGL()->GenRenderbuffers(1, &captureRBO);

        Core::GetEngine().GetGL()->BindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        Core::GetEngine().GetGL()->BindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        Core::GetEngine().GetGL()->RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, infos.width, infos.height);
        Core::GetEngine().GetGL()->FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

        equirectangularToCubemapShader->Activate();
        equirectangularToCubemapShader->SetInt("equirectangularMap", 0);
        equirectangularToCubemapShader->SetMat4("projection", captureProjection);
        Core::GetEngine().GetGL()->ActiveTexture(GL_TEXTURE0);
        Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_2D, hdrTex->GetID());

        Core::GetEngine().GetGL()->Viewport(0, 0, infos.width, infos.height); // Viewport => capture's dimensions
        Core::GetEngine().GetGL()->BindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        for (unsigned int i = 0; i < 6; ++i)
        {
            equirectangularToCubemapShader->SetMat4("view", captureViews[i]);
            Core::GetEngine().GetGL()->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemap->GetID(), 0);
            Core::GetEngine().GetGL()->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            RenderUnitCube(); // renders a 1x1 cube
        }
        Core::GetEngine().GetGL()->BindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void EnvironmentMap::CreateIrradiance(){

        std::shared_ptr<Shader> irradianceShader = Core::GetEngine().GetResourcesManager()->GetShader("shaders/ibl/irradiance");

        if(!irradianceShader)
        {
            DEBUG_ERROR("Cubemap irradiance map couldn't be created because shader \"shaders/ibl/irradiance\" is not loaded !");
            return;
        }
        
        unsigned char* data[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
        GLenum filters[2] = {GL_LINEAR, GL_LINEAR};
        GLenum wrapModes[3] = {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE};

        irradianceMap = std::make_shared<Cubemap>(32, 32, filters, wrapModes, GL_RGB16F, GL_RGB, data, false);

        Core::GetEngine().GetGL()->BindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        Core::GetEngine().GetGL()->BindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        Core::GetEngine().GetGL()->RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

        irradianceShader->Activate();
        irradianceShader->SetInt("environmentMap", 0);
        irradianceShader->SetMat4("projection", captureProjection);
        Core::GetEngine().GetGL()->ActiveTexture(GL_TEXTURE0);
        Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_CUBE_MAP, cubemap->GetID());

        Core::GetEngine().GetGL()->Viewport(0, 0, 32, 32);
        Core::GetEngine().GetGL()->BindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        for (unsigned int i = 0; i < 6; ++i)
        {
            irradianceShader->SetMat4("view", captureViews[i]);
            Core::GetEngine().GetGL()->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap->GetID(), 0);
            Core::GetEngine().GetGL()->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            RenderUnitCube();
        }
        Core::GetEngine().GetGL()->BindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void EnvironmentMap::CreatePrefilter(){

        std::shared_ptr<Shader> prefilterShader = Core::GetEngine().GetResourcesManager()->GetShader("shaders/ibl/prefilter");

        if(!prefilterShader)
        {
            DEBUG_ERROR("Cubemap prefilter map couldn't be created because shader \"shaders/ibl/equirectToCubemap\" is not loaded !");
            return;
        }

        unsigned char* data[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
        GLenum filters[2] = {GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR};
        GLenum wrapModes[3] = {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE};

        prefilterMap = std::make_shared<Cubemap>(128, 128, filters, wrapModes, GL_RGB16F, GL_RGB, data, true);

        prefilterShader->Activate();
        prefilterShader->SetInt("environmentMap", 0);
        prefilterShader->SetMat4("projection", captureProjection);
        Core::GetEngine().GetGL()->ActiveTexture(GL_TEXTURE0);
        Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_CUBE_MAP, cubemap->GetID());

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
            prefilterShader->SetFloat("roughness", roughness);
            for (unsigned int i = 0; i < 6; ++i)
            {
                prefilterShader->SetMat4("view", captureViews[i]);
                Core::GetEngine().GetGL()->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap->GetID(), mip);

                Core::GetEngine().GetGL()->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                RenderUnitCube();
            }
        }
        Core::GetEngine().GetGL()->BindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void EnvironmentMap::CreateBRDFLUT(){
        
        std::shared_ptr<Shader> brdfShader = Core::GetEngine().GetResourcesManager()->GetShader("shaders/ibl/brdf");

        if(!brdfShader)
        {
            DEBUG_ERROR("Cubemap brdf lookup table couldn't be created because shader \"shaders/ibl/brdf\" is not loaded !");
            return;
        }

        GLenum filters[2] = {GL_LINEAR, GL_LINEAR};
        GLenum wrapModes[2] = {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE};

        brdfLUT = std::make_shared<Texture>(512, 512, 2, filters, wrapModes, GL_RG16F, GL_RG, nullptr, false);

        // then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
        Core::GetEngine().GetGL()->BindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        Core::GetEngine().GetGL()->BindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        Core::GetEngine().GetGL()->RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
        Core::GetEngine().GetGL()->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUT->GetID(), 0);

        Core::GetEngine().GetGL()->Viewport(0, 0, 512, 512);
        brdfShader->Activate();
        Core::GetEngine().GetGL()->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        RenderUnitQuad();

        Core::GetEngine().GetGL()->BindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void EnvironmentMap::RenderUnitCube()
    {
        Core::GetEngine().GetGL()->BindVertexArray(cubeVAO);
        Core::GetEngine().GetGL()->DrawArrays(GL_TRIANGLES, 0, 36);
        Core::GetEngine().GetGL()->BindVertexArray(0);
    }

    void EnvironmentMap::RenderUnitQuad(){
        Core::GetEngine().GetGL()->BindVertexArray(quadVAO);
        Core::GetEngine().GetGL()->DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        Core::GetEngine().GetGL()->BindVertexArray(0);
    }

    void Cubemap::Bind(int unit)
    {
        Core::GetEngine().GetGL()->ActiveTexture(GL_TEXTURE0 + unit);
        Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_CUBE_MAP, ID);
    }

    void Cubemap::UnBind(int unit)
    {
        Core::GetEngine().GetGL()->ActiveTexture(GL_TEXTURE0 + unit);
        Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }

    void Cubemap::Cleanup()
    {
        Core::GetEngine().GetGL()->DeleteTextures(1, &ID);
    }
    
    void EnvironmentMap::GenerateGeometry()
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