#pragma once

#include "gl_cubemap.hpp"

#include "engine/rendering/texture/cubemap/envmap.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace Pulse::Engine::Rendering{

    class GLEnvironmentMap : public EnvironmentMap{
        public:

            GLEnvironmentMap(const TextureSpecifications& infos, std::shared_ptr<Cubemap> cubemap,
                std::shared_ptr<Cubemap> irradianceMap, std::shared_ptr<Cubemap> prefilterMap,
                std::shared_ptr<Texture2D> brdfLUT);

        private:
    };

    class GLEnvironmentMapGenerator : public EnvironmentMapGenerator{

        public:
            std::shared_ptr<EnvironmentMap> GenerateFromFiles(TextureSpecifications& specs, const std::vector<Filesystem::Path> imageFiles) override;
            std::shared_ptr<EnvironmentMap> GenerateFromHDR(TextureSpecifications& specs, const Filesystem::Path hdrFile) override;

        private:
            uint32_t captureFBO, captureRBO;
            uint32_t cubeVAO, cubeVBO;
            uint32_t quadVAO, quadVBO;

            glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
            glm::mat4 captureViews[6] = {
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
            };

            void GenerateGeometry();
            void CreateIrradiance(std::shared_ptr<Cubemap> irradianceMap, std::shared_ptr<Cubemap> cubemap);
            void CreatePrefilter(std::shared_ptr<Cubemap> prefilterMap, std::shared_ptr<Cubemap> cubemap);
            void CreateBRDFLUT(std::shared_ptr<Texture2D> brdfLUT);

            void RenderUnitCube();
            void RenderUnitQuad();
    };

}