#pragma once

#include <memory>

#include "engine/rendering/utils.hpp"

namespace Pulse::Engine::Rendering {

    enum LightType : uint32_t{
        Directional = 0,
        Point = 1,
        Spot = 2
    };

    struct LightData {
        alignas(4) int type = 0;
        alignas(4) float intensity= 1.0f;
        alignas(16) glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
        alignas(16) glm::vec3 direction = glm::vec3(0.0f, 0.0f, 0.0f);
        alignas(4) float radius = 100;
        alignas(16) glm::vec3 color = COL_RGB(1.0f);
        alignas(4) float innerCutoff = glm::cos(glm::radians(1.5f));
        alignas(4) float outerCutoff = glm::cos(glm::radians(7.5f));
        alignas(4) bool castShadow = true;

        glm::mat4 GetLightMatrix(const glm::mat4 &cameraView = glm::mat4(1.0f), const float fov = -1.0f, const float aspectRatio = -1.0f, const float cascadeNear = -1.0f, const float cascadeFar = -1.0f, const float shadowRes = -1.0);
    };

    class LightManager {
        public:
            LightManager();

            ~LightManager();

            void Update(int updatedLightIndex);

            void AddLight(int lightIndex, std::shared_ptr<LightData> light);

            void Clear();

            void RemoveLight(int lightIndex);

            int GetLightsCount();

        private:
            std::vector<std::shared_ptr<LightData>> lights;
    };
}