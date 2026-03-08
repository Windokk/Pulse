#include "light_manager.hpp"

#include <type_traits>
#include <iostream>

#include "engine/rendering/renderer/renderer.hpp"


#include <glm/gtx/string_cast.hpp>

#include "engine/core/engine.hpp"

#include "engine/rendering/opengl/opengl.hpp"

namespace Pulse::Engine::Rendering{
    
    LightManager::LightManager()
    {
        Core::GetEngine().GetGL()->GenBuffers(1, &ssbo);
    }

    LightManager::~LightManager()
    {
        Core::GetEngine().GetGL()->DeleteBuffers(1, &ssbo);
    }

    /// @note Only call this AFTER modifying the light data
    /// @brief Update the light system's storage buffer, and shadow maps
    /// @param updatedLight The global index (scene-relative) of the modified light
    void LightManager::Update(int updatedLight){

        if(lights.size() != 0){
            std::vector<LightData> flatLights;
            flatLights.reserve(lights.size());

            for (std::shared_ptr<LightData> light : lights) {
                if (light) {
                    flatLights.push_back(*light.get());
                }
            }

            Core::GetEngine().GetGL()->BindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
            Core::GetEngine().GetGL()->BufferData(GL_SHADER_STORAGE_BUFFER, sizeof(LightData) * flatLights.size(), flatLights.data(), GL_DYNAMIC_DRAW);
            Core::GetEngine().GetGL()->BindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
        }

        if(updatedLight == -1)
            return;

        Core::GetEngine().GetRenderer()->shadowMan->RegisterLight(updatedLight, lights[updatedLight]);
    }

    /// @brief Add a light to the renderer
    /// @param index The global index (scene-relative) of the light
    /// @param light The light's parameters
    void LightManager::AddLight(int index, std::shared_ptr<LightData> light)
    {
        lights.push_back(light);
    }

    /// @brief Remove all lights from the renderer (and their associated shadow maps)
    void LightManager::Clear()
    {
        for (int i = 0; i < lights.size(); ++i)
        {
            if (lights[i] && lights[i]->castShadow)
            {
                Core::GetEngine().GetRenderer()->shadowMan->UnregisterLight(i);
            }
        }

        lights.clear();
    }

    /// @brief Remove light from renderer (and its associated shadow map)
    /// @param lightIndex The global (scene-relative) light index to remove
    void LightManager::RemoveLight(int lightIndex)
    {
        if (lightIndex < 0 || lightIndex >= static_cast<int>(lights.size()))
            return;

        auto* renderer = Core::GetEngine().GetRenderer();
        auto* shadowMan = renderer->shadowMan;

        if (lights[lightIndex] && lights[lightIndex]->castShadow)
        {
            shadowMan->UnregisterLight(lightIndex);
        }

        lights.erase(lights.begin() + lightIndex);

        shadowMan->ClearAll();

        for (size_t i = 0; i < lights.size(); ++i)
        {
            if (lights[i] && lights[i]->castShadow)
            {
                shadowMan->RegisterLight(static_cast<int>(i), lights[i]);
            }
        }
    }

    /// @brief Getter for lights count
    /// @return The total number of lights in the renderer
    int LightManager::GetLightsCount()
    {
        return lights.size();
    }
    
    
    std::vector<glm::vec4> getFrustumCornersWorldSpace(const glm::mat4& projview)
    {
        const auto inv = glm::inverse(projview);

        std::vector<glm::vec4> frustumCorners;
        for (unsigned int x = 0; x < 2; ++x)
        {
            for (unsigned int y = 0; y < 2; ++y)
            {
                for (unsigned int z = 0; z < 2; ++z)
                {
                    const glm::vec4 pt = inv * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
                    frustumCorners.push_back(pt / pt.w);
                }
            }
        }

        return frustumCorners;
    }

    /// @brief Getter for lights matrices
    /// @return The view-projection matrix from the light's point of view
    glm::mat4 LightData::GetLightMatrix(const glm::mat4& cameraView, const float fov, const float aspectRatio, const float cascadeNear, const float cascadeFar, const float shadowRes)
    {
        if (type == static_cast<int>(LightType::Directional) && cascadeFar != -1 && fov != -1 && aspectRatio != -1 && cascadeFar != -1)
        {
            const auto proj = glm::perspective(
                glm::radians(fov), aspectRatio, cascadeNear,
                cascadeFar);
            const auto corners = getFrustumCornersWorldSpace(proj * cameraView);

            //Frustum center
            glm::vec3 center(0.0f);
            for (auto& v : corners)
                center += glm::vec3(v);
            center /= corners.size();

            glm::vec3 up(0, 1, 0);
            if (fabs(glm::dot(direction, up)) > 0.99f)
                up = glm::vec3(1, 0, 0);
            const auto lightView = glm::lookAt(center + direction, center, up);

            float minX = std::numeric_limits<float>::max();
            float maxX = std::numeric_limits<float>::lowest();
            float minY = std::numeric_limits<float>::max();
            float maxY = std::numeric_limits<float>::lowest();
            float minZ = std::numeric_limits<float>::max();
            float maxZ = std::numeric_limits<float>::lowest();
            for (const auto& v : corners)
            {
                const auto trf = lightView * v;
                minX = std::min(minX, trf.x);
                maxX = std::max(maxX, trf.x);
                minY = std::min(minY, trf.y);
                maxY = std::max(maxY, trf.y);
                minZ = std::min(minZ, trf.z);
                maxZ = std::max(maxZ, trf.z);
            }

            // Tune this parameter according to the scene
            constexpr float zMult = 10.0f;
            if (minZ < 0)
            {
                minZ *= zMult;
            }
            else
            {
                minZ /= zMult;
            }
            if (maxZ < 0)
            {
                maxZ /= zMult;
            }
            else
            {
                maxZ *= zMult;
            }

            const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, maxZ, minZ);

            return lightProjection * lightView;
        }
        else if (type == static_cast<int>(LightType::Spot))
        {
            float orthoSize = 10.0f;
            glm::mat4 proj = glm::perspective(glm::radians(60.0f), static_cast<float>(Core::GetEngine().GetWindow()->GetFramebufferWidth()/Core::GetEngine().GetWindow()->GetFramebufferHeight()), 0.1f, radius);

            glm::vec3 lightDir = glm::normalize(direction);
            glm::vec3 lightPos = position;

            glm::vec3 up = glm::abs(glm::dot(lightDir, glm::vec3(0, 1, 0))) > 0.99f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
            glm::mat4 view = glm::lookAt(lightPos, lightPos+lightDir, up);

            return proj * view;
        }

        // For point lights : returns identity matrix
        return glm::mat4(1.0f);
    }
}