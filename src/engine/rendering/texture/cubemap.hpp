#pragma once

#include "engine/ecs/components/core/transform.hpp"

#include <stb/stb_image.h>
#include <stb/stb_image_resize2.h>
#include <string>
#include <memory>

namespace Pulse::Engine::Filesystem{

    class Path;
}

namespace Pulse::Engine::Rendering {

    struct DrawCommand;

    struct CubeVertex{
        glm::vec3 pos;
    };

    struct CubemapInfos{
        int width, height;       
        int nrChannels;
        std::shared_ptr<Filesystem::Path> filepath;
    };

    class Shader;

    class Cubemap{
        public:
            Cubemap(Filesystem::Path filepath);
            void Draw(std::shared_ptr<Shader> shader, glm::mat4 view, glm::mat4 projection);
            void Init(Filesystem::Path filepath);
            void Bind();
            void UnBind();
            void Cleanup();
            unsigned int GetID() { return cubemapID; }
            unsigned int GetIrradianceID() { return irradianceMapID; }
            unsigned int GetPrefilterID() { return prefilterMapID; }
            unsigned int GetBrdfLutID() { return brdfLUTTextureID; }
            CubemapInfos* GetInfos() { return &infos; }

        private:
        
            void GenerateGeometry();
            void CreateFromFolder();
            void CreateFromHDR();
            void CreateIrradiance();
            void CreatePrefilter();
            void CreateBRDFLUT();
            void RenderUnitCube();
            void RenderUnitQuad();
            unsigned int cubemapID;
            unsigned int irradianceMapID;
            unsigned int prefilterMapID;
            unsigned int brdfLUTTextureID;
            unsigned int captureFBO, captureRBO;
            CubemapInfos infos;
            unsigned int cubeVAO, cubeVBO;
            unsigned int quadVAO, quadVBO;

            glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
            glm::mat4 captureViews[6] = {
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
            };
    };
}