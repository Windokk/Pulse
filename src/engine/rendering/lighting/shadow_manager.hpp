#pragma once

#include "engine/rendering/lighting/light_manager.hpp"

#define MAX_DIRECTIONAL_LIGHTS 3
#define CASCADES_PER_LIGHT 3
#define NUM_CASCADES (MAX_DIRECTIONAL_LIGHTS * CASCADES_PER_LIGHT)

namespace Pulse::Engine::ECS::Components{
    class Camera;
}

namespace Pulse::Engine::Rendering {

    class Shader;
    class Material;
    class Mesh;

    struct ShadowMap {
        LightData light;

        // Directional and spot
        GLuint fbo[CASCADES_PER_LIGHT] = {0};       // One FBO per cascade (only 1 used for spot)
        GLuint depthMap[CASCADES_PER_LIGHT] = {0};  // One shadow map per cascade
        GLuint resolveDepthMap[CASCADES_PER_LIGHT] = { 0 };
        glm::mat4 lightMatrix[CASCADES_PER_LIGHT];  // One matrix per cascade
        float cascadeSplits[CASCADES_PER_LIGHT];    // Only used for directional lights

        // Point
        GLuint depthCubemap = 0;              // Single cubemap array for point lights
        glm::mat4 shadowMatrices[6];          // 6 face matrices (point lights)
        int cubeArrayLayer = -1;

        int resolution = 0;
    };

    class ShadowManager {
    public:
        void Init(int pointShadowsResolution, int spotShadowsResolution, int dirShadowsResolution);
        void RegisterLight(int lightIndex, std::shared_ptr<LightData> light);
        
        void UnregisterLight(int lightIndex);
        void RenderShadowMaps(const std::vector<std::pair<glm::mat4, Rendering::Mesh *>> &meshes, std::shared_ptr<ECS::Components::Camera> cam);
        void BindShadowMaps(std::shared_ptr<Pulse::Engine::Rendering::Material> material);
        void ClearAll();

        int DIR_SHADOW_BASE_UNIT   = 11; // NUM_CASCADES units
        int SPOT_SHADOW_BASE_UNIT  = 20; // MAX_SPOT_LIGHTS units
        int POINT_SHADOW_UNIT      = 30; // 1 cube map array

    private:

        void EnsureCubeArrayCapacity(int requiredPointLights);
        void TryShrinkCubeArray();

        std::vector<ShadowMap> shadowMaps;
        std::shared_ptr<Shader> dirShader;
        std::shared_ptr<Shader> spotShader;
        std::shared_ptr<Shader> pointShader;
        int pointShadowsResolution = 0;
        int spotShadowsResolution = 0;
        int dirShadowsResolution = 0;
        GLuint cubeArrayTex;
        int pointLightCount = 0;
        int currentCapacity = 0;
    };
}