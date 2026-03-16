#include "gl_envmap.hpp"

#include "engine/core/engine.hpp"

#include "engine/rendering/backends/opengl/gl_utils.hpp"

#include "engine/rendering/shader/shader.hpp"

#include <stb/stb_image.h>

#include "engine/core/resources/resources_manager.hpp"

#include "engine/rendering/backends/opengl/shader/gl_shader.hpp"

namespace Pulse::Engine::Rendering{

    std::shared_ptr<EnvironmentMap> GLEnvironmentMapGenerator::GenerateFromFiles(TextureSpecifications &specs, const std::vector<Filesystem::Path> imageFiles)
    {
        if(imageFiles.size() < 6)
            return nullptr;

        std::array<Filesystem::Path, 6> files = {imageFiles[0], imageFiles[1], imageFiles[2], imageFiles[3], imageFiles[4], imageFiles[5]};
        std::shared_ptr<Cubemap> cubemap = Cubemap::Create(specs, files);

        std::shared_ptr<Cubemap> irradianceMap;
        std::shared_ptr<Cubemap> prefilterMap;
        std::shared_ptr<Texture2D> brdfLUT;

        GenerateGeometry();

        std::array<unsigned char*, 6> dataArray = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};

        TextureSpecifications texSpecs;

        texSpecs.magFilter = TextureFilter::Linear;
        texSpecs.minFilter = TextureFilter::Linear;
        texSpecs.wrapS = TextureWrap::ClampBorder;
        texSpecs.wrapT = TextureWrap::ClampBorder;
        texSpecs.wrapR = TextureWrap::ClampBorder;
        texSpecs.generateMips = false;
        texSpecs.format = TextureFormat::RGB;
        texSpecs.width = 32;
        texSpecs.height = 32;

        irradianceMap = Cubemap::Create(texSpecs, dataArray);

        CreateIrradiance(irradianceMap, cubemap);

        texSpecs.magFilter = TextureFilter::Linear;
        texSpecs.minFilter = TextureFilter::Linear;
        texSpecs.wrapS = TextureWrap::ClampBorder;
        texSpecs.wrapT = TextureWrap::ClampBorder;
        texSpecs.wrapR = TextureWrap::ClampBorder;
        texSpecs.generateMips = true;
        texSpecs.format = TextureFormat::RGB;
        texSpecs.width = 128;
        texSpecs.height = 128;

        prefilterMap = Cubemap::Create(texSpecs, dataArray);

        CreatePrefilter(prefilterMap, cubemap);

        texSpecs.magFilter = TextureFilter::Linear;
        texSpecs.minFilter = TextureFilter::Linear;
        texSpecs.wrapS = TextureWrap::ClampBorder;
        texSpecs.wrapT = TextureWrap::ClampBorder;
        texSpecs.wrapR = TextureWrap::ClampBorder;
        texSpecs.generateMips = false;
        texSpecs.format = TextureFormat::RG;
        texSpecs.width = 512;
        texSpecs.height = 512;

        brdfLUT = Texture2D::Create(texSpecs, nullptr);

        CreateBRDFLUT(brdfLUT);

        return std::make_shared<GLEnvironmentMap>(specs, cubemap, irradianceMap, prefilterMap, brdfLUT);
    }

    std::shared_ptr<EnvironmentMap> GLEnvironmentMapGenerator::GenerateFromHDR(TextureSpecifications &specs, const Filesystem::Path hdrFile)
    {
        //Extract cubemap data from the hdr file
        std::shared_ptr<Texture2D> hdrTex = Texture2D::Create(specs, hdrFile);

        specs.width = hdrTex->GetWidth();
        specs.height = hdrTex->GetHeight();
        specs.format = hdrTex->GetSpecifications().format;

        std::shared_ptr<Shader> equirectangularToCubemapShaderAbstract = Core::GetEngine().GetResourcesManager()->GetShader("shaders/ibl/equirectToCubemap");

        std::shared_ptr<GLShader> equirectangularToCubemapShader = std::static_pointer_cast<GLShader>(equirectangularToCubemapShaderAbstract);

        std::array<unsigned char*, 6> dataArray = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
        std::shared_ptr<Cubemap> cubemap = Cubemap::Create(specs, dataArray);
        {
            if(!equirectangularToCubemapShader)
            {
                DEBUG_ERROR("HDR cubemap couldn't be created because shader \"shaders/ibl/equirectToCubemap\" is not loaded !");
                return nullptr;
            }

            glGenFramebuffers(1, &captureFBO);
            glGenRenderbuffers(1, &captureRBO);

            glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
            glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, specs.width, specs.height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

            equirectangularToCubemapShader->Bind();
            equirectangularToCubemapShader->SetInt("equirectangularMap", 0);
            equirectangularToCubemapShader->SetMat4("projection", captureProjection);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, hdrTex->GetHandle());

            glViewport(0, 0, specs.width, specs.height); // Viewport => capture's dimensions
            glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
            for (unsigned int i = 0; i < 6; ++i)
            {
                equirectangularToCubemapShader->SetMat4("view", captureViews[i]);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemap->GetHandle(), 0);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                RenderUnitCube(); // renders a 1x1 cube
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    
        std::shared_ptr<Cubemap> irradianceMap;
        std::shared_ptr<Cubemap> prefilterMap;
        std::shared_ptr<Texture2D> brdfLUT;

        GenerateGeometry();

        TextureSpecifications texSpecs;

        texSpecs.magFilter = TextureFilter::Linear;
        texSpecs.minFilter = TextureFilter::Linear;
        texSpecs.wrapS = TextureWrap::ClampBorder;
        texSpecs.wrapT = TextureWrap::ClampBorder;
        texSpecs.wrapR = TextureWrap::ClampBorder;
        texSpecs.generateMips = false;
        texSpecs.format = TextureFormat::RGB;
        texSpecs.width = 32;
        texSpecs.height = 32;

        irradianceMap = Cubemap::Create(texSpecs, dataArray);

        CreateIrradiance(irradianceMap, cubemap);

        texSpecs.magFilter = TextureFilter::Linear;
        texSpecs.minFilter = TextureFilter::Linear;
        texSpecs.wrapS = TextureWrap::ClampBorder;
        texSpecs.wrapT = TextureWrap::ClampBorder;
        texSpecs.wrapR = TextureWrap::ClampBorder;
        texSpecs.generateMips = true;
        texSpecs.format = TextureFormat::RGB;
        texSpecs.width = 128;
        texSpecs.height = 128;

        prefilterMap = Cubemap::Create(texSpecs, dataArray);

        CreatePrefilter(prefilterMap, cubemap);

        texSpecs.magFilter = TextureFilter::Linear;
        texSpecs.minFilter = TextureFilter::Linear;
        texSpecs.wrapS = TextureWrap::ClampBorder;
        texSpecs.wrapT = TextureWrap::ClampBorder;
        texSpecs.wrapR = TextureWrap::ClampBorder;
        texSpecs.generateMips = false;
        texSpecs.format = TextureFormat::RG;
        texSpecs.width = 512;
        texSpecs.height = 512;

        brdfLUT = Texture2D::Create(texSpecs, nullptr);

        CreateBRDFLUT(brdfLUT);

        return std::make_shared<GLEnvironmentMap>(specs, cubemap, irradianceMap, prefilterMap, brdfLUT);
    }

    void GLEnvironmentMapGenerator::GenerateGeometry()
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

        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        glBindVertexArray(cubeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

        glBindVertexArray(0);

        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }

    void GLEnvironmentMapGenerator::CreateIrradiance(std::shared_ptr<Cubemap> irradianceMap, std::shared_ptr<Cubemap> cubemap)
    {
        std::shared_ptr<Shader> irradianceShaderAbstract = Core::GetEngine().GetResourcesManager()->GetShader("shaders/ibl/irradiance");

        std::shared_ptr<GLShader> irradianceShader = std::static_pointer_cast<GLShader>(irradianceShaderAbstract);

        if(!irradianceShader)
        {
            DEBUG_ERROR("Cubemap irradiance map couldn't be created because shader \"shaders/ibl/irradiance\" is not loaded !");
            return;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

        irradianceShader->Bind();
        irradianceShader->SetInt("environmentMap", 0);
        irradianceShader->SetMat4("projection", captureProjection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap->GetHandle());

        glViewport(0, 0, 32, 32);
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        for (unsigned int i = 0; i < 6; ++i)
        {
            irradianceShader->SetMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap->GetHandle(), 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            RenderUnitCube();
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GLEnvironmentMapGenerator::CreatePrefilter(std::shared_ptr<Cubemap> prefilterMap, std::shared_ptr<Cubemap> cubemap)
    {
        std::shared_ptr<Shader> prefilterShaderAbstract = Core::GetEngine().GetResourcesManager()->GetShader("shaders/ibl/prefilter");

        std::shared_ptr<GLShader> prefilterShader = std::static_pointer_cast<GLShader>(prefilterShaderAbstract);

        if(!prefilterShader)
        {
            DEBUG_ERROR("Cubemap prefilter map couldn't be created because shader \"shaders/ibl/equirectToCubemap\" is not loaded !");
            return;
        }

        prefilterShader->Bind();
        prefilterShader->SetInt("environmentMap", 0);
        prefilterShader->SetMat4("projection", captureProjection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap->GetHandle());

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        unsigned int maxMipLevels = 5;
        for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
        {
            // reisze framebuffer according to mip-level size.
            unsigned int mipWidth  = static_cast<unsigned int>(128 * std::pow(0.5, mip));
            unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));
            glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
            glViewport(0, 0, mipWidth, mipHeight);

            float roughness = (float)mip / (float)(maxMipLevels - 1);
            prefilterShader->SetFloat("roughness", roughness);
            for (unsigned int i = 0; i < 6; ++i)
            {
                prefilterShader->SetMat4("view", captureViews[i]);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap->GetHandle(), mip);

                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                RenderUnitCube();
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GLEnvironmentMapGenerator::CreateBRDFLUT(std::shared_ptr<Texture2D> brdfLUT)
    {
        std::shared_ptr<Shader> brdfShaderAbstract = Core::GetEngine().GetResourcesManager()->GetShader("shaders/ibl/brdf");

        std::shared_ptr<GLShader> brdfShader = std::static_pointer_cast<GLShader>(brdfShaderAbstract);

        if(!brdfShader)
        {
            DEBUG_ERROR("Cubemap brdf lookup table couldn't be created because shader \"shaders/ibl/brdf\" is not loaded !");
            return;
        }

        // then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUT->GetHandle(), 0);

        glViewport(0, 0, 512, 512);
        brdfShader->Bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        RenderUnitQuad();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GLEnvironmentMapGenerator::RenderUnitCube()
    {
        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }

    void GLEnvironmentMapGenerator::RenderUnitQuad()
    {
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }

    GLEnvironmentMap::GLEnvironmentMap(const TextureSpecifications &infos, std::shared_ptr<Cubemap> cubemap, std::shared_ptr<Cubemap> irradianceMap, std::shared_ptr<Cubemap> prefilterMap, std::shared_ptr<Texture2D> brdfLUT)
    {
        m_Infos = infos;
        m_Cubemap = cubemap;
        m_IrradianceMap = irradianceMap;
        m_PrefilterMap = prefilterMap;
        m_BrdfLUT = brdfLUT;
    }
}