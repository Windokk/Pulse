#pragma once

#include "light_manager.hpp"

namespace Pulse::Engine::ECS::Components{
    class Camera;
}

namespace Pulse::Engine::Rendering {

    class Shader;
    class Material;
    class Mesh;
    class Texture2D;
    class Cubemap;
    class CubemapArray;
    class Framebuffer;
    
    static constexpr uint32_t CASCADES_PER_LIGHT = 3;
    static constexpr uint32_t NUM_CASCADES = (LightManager::MAX_DIRECTIONAL_LIGHTS * CASCADES_PER_LIGHT);

    struct ShadowMap {
        LightData light;

        // Directional and spot
        std::shared_ptr<Framebuffer> framebuffer[CASCADES_PER_LIGHT] = {nullptr};       // One FBO per cascade (only 1 used for spot)
        glm::mat4 lightMatrix[CASCADES_PER_LIGHT];                                      // One matrix per cascade
        float cascadeSplits[CASCADES_PER_LIGHT];                                        // Only used for directional lights

        // Point 
        std::shared_ptr<Cubemap> depthCubemap;                                          // Single cubemap for point lights
        glm::mat4 shadowMatrices[6];                                                    // 6 face matrices (point lights)
        int cubeArrayLayer = -1;

        int resolution = 0;
    };

    class ShadowManager {
    public:

        /// @brief Initializer for the shadow manager
        /// @param resolution The resolution to use for shadow maps
        void Init(int pointShadowsResolution, int spotShadowsResolution, int dirShadowsResolution);
        
        /// @brief Register/Replace a light for shadow mappings
        /// @param lightIndex The updated/new light's global index in the scene
        /// @param light The point to the light's data
        void RegisterLight(int lightIndex, std::shared_ptr<LightData> light);
        
        /// @brief Remove a light from shadow mappings
        /// @param lightIndex The light's global index in the scene
        void UnregisterLight(int lightIndex);

        /// @brief Bind the shadow maps to a material
        /// @param material The material to bind shadow maps to
        void BindShadowMaps(std::shared_ptr<Pulse::Engine::Rendering::Material> material);

        void ClearAll();

        int DIR_SHADOW_BASE_UNIT   = 11; // NUM_CASCADES units
        int SPOT_SHADOW_BASE_UNIT  = 20; // MAX_SPOT_LIGHTS units
        int POINT_SHADOW_UNIT      = 30; // 1 cube map array

        const int GetShadowMapsCount() const {
            {
                int count = 0;

                for (const auto& map : m_ShadowMaps)
                {
                    switch (map.light.type)
                    {
                        case LightType::Directional:
                            count += CASCADES_PER_LIGHT;
                            break;

                        case LightType::Spot:
                            count += 1;
                            break;

                        case LightType::Point:
                            count += 1; // cubemap array layer
                            break;
                    }
                }

                return count;
            } 
        }

        void UpdatePassUniforms();
        
    private:

        void SubmitPasses();
        void EnsureCubeArrayCapacity(int requiredPointLights);
        void TryShrinkCubeArray();

        std::vector<ShadowMap> m_ShadowMaps;
        std::shared_ptr<Shader> m_DirShader;
        std::shared_ptr<Shader> m_SpotShader;
        std::shared_ptr<Shader> m_PointShader;
        int m_PointShadowsResolution = 0;
        int m_SpotShadowsResolution = 0;
        int m_DirShadowsResolution = 0;
        std::shared_ptr<CubemapArray> m_CubeArrayTex; 
        int m_PointLightCount = 0;
        int m_CurrentPointLightCapacity = 0;
    };
}