#include "light_manager.hpp"

#include <type_traits>
#include <iostream>

#include "engine/rendering/renderer/renderer.hpp"
#include <glm/gtx/string_cast.hpp>

#include "engine/core/engine.hpp"

#include "engine/rendering/opengl/opengl.hpp"

namespace Epoch::Engine::Rendering{
    
    LightManager::LightManager()
    {
        GetGL().GenBuffers(1, &ssbo);
    }

    LightManager::~LightManager()
    {
        GetGL().DeleteBuffers(1, &ssbo);
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

            GetGL().BindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
            GetGL().BufferData(GL_SHADER_STORAGE_BUFFER, sizeof(LightData) * flatLights.size(), flatLights.data(), GL_DYNAMIC_DRAW);
            GetGL().BindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
        }

        if(updatedLight == -1)
            return;

        Renderer::GetInstance().shadowMan->RegisterLight(updatedLight, lights[updatedLight]);
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
                Renderer::GetInstance().shadowMan->UnregisterLight(i);
            }
        }

        lights.clear();
    }

    /// @brief Remove light from renderer (and its associated shadow map)
    /// @param lightIndex The global (scene-relative) light index to remove
    void LightManager::RemoveLight(int lightIndex)
    {
        if (lightIndex < 0 || lightIndex >= lights.size())
            return;

        if (lights[lightIndex] && lights[lightIndex]->castShadow)
        {
            Renderer::GetInstance().shadowMan->UnregisterLight(lightIndex);
        }

        lights.erase(lights.begin() + lightIndex);

        for (int i = 0; i < lights.size(); ++i)
        {
            if (lights[i] && lights[i]->castShadow)
            {
                Renderer::GetInstance().shadowMan->RegisterLight(i, lights[i]);
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
            for (const auto& v : corners)
                center += glm::vec3(v);
            center /= corners.size();

            //Light view
            glm::vec3 lightDir = glm::normalize(direction);
            glm::vec3 up = (fabs(glm::dot(direction, glm::vec3(0,1,0))) > 0.99f) ? glm::vec3(1,0,0) : glm::vec3(0,1,0);
            glm::mat4 lightView = glm::lookAt(center - lightDir, center, up);

            //Frustum "radius" computation
            float radius = 0.0f;
            for (const auto& v : corners) {
                float dist = glm::length(glm::vec3(v) - center);
                radius = glm::max(radius, dist);
            }
            radius = std::ceil(radius);

            //AABB from radius
            glm::vec3 maxOrtho = center + glm::vec3(radius);
            glm::vec3 minOrtho = center - glm::vec3(radius);

            maxOrtho = glm::vec3(lightView * glm::vec4(maxOrtho, 1.0f));
            minOrtho = glm::vec3(lightView * glm::vec4(minOrtho, 1.0f));

            float zMult = (cascadeFar - cascadeNear);
            float nearPlane = minOrtho.z - zMult;
            float farPlane = maxOrtho.z + zMult;

            glm::mat4 lightProjection = glm::ortho(minOrtho.x, maxOrtho.x,
                                                minOrtho.y, maxOrtho.y,
                                                nearPlane, farPlane);

            // Texel snaapping
            glm::mat4 shadowMatrix = lightProjection * lightView;
            glm::vec4 shadowOrigin = shadowMatrix * glm::vec4(0, 0, 0, 1);
            float shadowMapSize = shadowRes;
            shadowOrigin = shadowOrigin * (shadowMapSize / 2.0f);
            glm::vec4 roundedOrigin = glm::round(shadowOrigin);
            glm::vec4 roundOffset = roundedOrigin - shadowOrigin;
            roundOffset = roundOffset * (2.0f / shadowMapSize);
            roundOffset.z = 0.0f;
            roundOffset.w = 0.0f;
            lightProjection[3] += roundOffset;

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