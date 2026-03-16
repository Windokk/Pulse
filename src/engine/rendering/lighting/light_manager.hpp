#pragma once

#include <memory>

#include "engine/rendering/utils.hpp"

namespace Pulse::Engine::Rendering {

    class StorageBuffer;

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

        /// @brief Getter for lights matrices
        /// @return The view-projection matrix from the light's point of view
        glm::mat4 GetLightMatrix(const glm::mat4 &cameraView = glm::mat4(1.0f), const float fov = -1.0f, const float aspectRatio = -1.0f, const float cascadeNear = -1.0f, const float cascadeFar = -1.0f, const float shadowRes = -1.0);
    };

    class LightManager {
        public:

            static constexpr uint32_t MAX_LIGHTS = 128;

            static constexpr uint32_t MAX_DIRECTIONAL_LIGHTS = 3;

            LightManager();

            ~LightManager();

            /// @note Only call this AFTER modifying the light data
            /// @brief Update the light system's storage buffer, and shadow maps
            /// @param index The global index (level-relative) of the modified light
            void Update(int index);

            /// @brief Add a light to the renderer
            /// @param index The global index (level-relative) of the light
            /// @param light The light's parameters
            void AddLight(int lightIndex, std::shared_ptr<LightData> data);

            /// @brief Remove all lights from the renderer (and their associated shadow maps)
            void Clear();

            /// @brief Remove light from renderer (and its associated shadow map)
            /// @param lightIndex The global (level-relative) light index to remove
            void RemoveLight(int lightIndex);

            /// @brief Getter for lights count
            /// @return The total number of lights in the renderer
            int GetLightsCount();

        private:
            std::vector<std::shared_ptr<LightData>> m_Lights;

            std::shared_ptr<StorageBuffer> m_SSBO;
    };
}