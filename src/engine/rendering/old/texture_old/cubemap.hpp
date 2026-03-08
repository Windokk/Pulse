#pragma once

#include "engine/ecs/components/misc/transform.hpp"
#include "engine/filesystem/assetID.hpp"

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

    struct EnvironmentMapInfos{
        int width, height;       
        int nrChannels;
        std::shared_ptr<Filesystem::Path> filepath;
    };

    class Shader;

    class Texture;

    class Cubemap{
        public:
            Cubemap(int width, int height, GLenum filterMode[2], GLenum wrapMode[3], GLenum internalFormat, GLenum format, unsigned char *data[6], bool generateMipmap);
            Cubemap(unsigned int ID);
            void Draw(int vao, std::shared_ptr<Shader> shader, glm::mat4 view, glm::mat4 projection);
            void Bind(int unit);
            void UnBind(int unit);
            void Cleanup();

            unsigned int GetID() { return ID; }

        private:
            unsigned int ID;
    };

    class EnvironmentMap{
        public:
            EnvironmentMap(Filesystem::Path filepath);
            void Init(Filesystem::Path filepath);
            void Draw(std::shared_ptr<Shader> shader, glm::mat4 view, glm::mat4 projection);
            void Cleanup();

            std::shared_ptr<Cubemap> cubemap;
            std::shared_ptr<Cubemap> irradianceMap;
            std::shared_ptr<Cubemap> prefilterMap;
            std::shared_ptr<Texture> brdfLUT;

            EnvironmentMapInfos* GetInfos() { return &infos; }

            void SetAssetID(Filesystem::AssetID assetID) {
                this->assetID = assetID;
            }

            Filesystem::AssetID GetAssetID() {
                return this->assetID;
            }
        private:
        
            void GenerateGeometry();
            void CreateFromFolder();
            void CreateFromHDR();
            void CreateIrradiance();
            void CreatePrefilter();
            void CreateBRDFLUT();
            void RenderUnitCube();
            void RenderUnitQuad();

            Filesystem::AssetID assetID;

            unsigned int captureFBO, captureRBO;
            EnvironmentMapInfos infos;
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