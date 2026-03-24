#include "light_manager.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include "engine/core/engine.hpp"
#include "engine/core/platform/iwindow.hpp"

#include "engine/rendering/renderer/renderer.hpp"

#include "engine/rendering/lighting/shadow_manager.hpp"

#include "engine/rendering/buffer/storage_buffer.hpp"

namespace Pulse::Engine::Rendering{

    LightManager::LightManager()
    {
        m_SSBO = StorageBuffer::Create(sizeof(LightData) * 0);
    }

    LightManager::~LightManager()
    {
        m_SSBO->~StorageBuffer();
    }

    void LightManager::Update(int index)
    {
        if(m_Lights.empty())
            return;

        std::vector<LightData> flatLights;
        flatLights.reserve(m_Lights.size());

        for (auto& light : m_Lights)
        {
            if (light)
                flatLights.push_back(*light);
        }

        m_SSBO->SetData(flatLights.data(), sizeof(LightData) * flatLights.size());
        m_SSBO->Bind(0);

        if(index == -1)
            return;

        Core::GetEngine().GetRenderer()->GetShadowManager()->RegisterLight(index, m_Lights[index]);
    }

    void LightManager::AddLight(int index, std::shared_ptr<LightData> data)
    {
        m_Lights.push_back(data);
    }

    void LightManager::Clear()
    {
        for (int i = 0; i < m_Lights.size(); ++i)
        {
            if (m_Lights[i] && m_Lights[i]->castShadow)
            {
                Core::GetEngine().GetRenderer()->GetShadowManager()->UnregisterLight(i);
            }
        }

        m_Lights.clear();
    }

    void LightManager::RemoveLight(int lightIndex)
    {
        if (lightIndex < 0 || lightIndex >= static_cast<int>(m_Lights.size()))
            return;

        auto* renderer = Core::GetEngine().GetRenderer();
        auto shadowMan = renderer->GetShadowManager();

        if (m_Lights[lightIndex] && m_Lights[lightIndex]->castShadow)
        {
            shadowMan->UnregisterLight(lightIndex);
        }

        m_Lights.erase(m_Lights.begin() + lightIndex);

        shadowMan->ClearAll();

        for (size_t i = 0; i < m_Lights.size(); ++i)
        {
            if (m_Lights[i] && m_Lights[i]->castShadow)
            {
                shadowMan->RegisterLight(static_cast<int>(i), m_Lights[i]);
            }
        }
    }

    int LightManager::GetLightsCount()
    {
        return m_Lights.size();
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

            
            float radius = 0.0f;
            for (const auto& v : corners)
                radius = std::max(radius, glm::length(glm::vec3(v) - center));

            // snap radius to reduce flicker
            radius = std::ceil(radius * 16.0f) / 16.0f;

            // ight direction
            glm::vec3 lightDir = glm::normalize(direction);

            // robust up vector
            glm::vec3 up = fabs(glm::dot(lightDir, glm::vec3(0,1,0))) > 0.99f
                ? glm::vec3(1,0,0)
                : glm::vec3(0,1,0);

            // Build light view
            glm::vec3 lightPos = center - lightDir * radius;
            glm::mat4 lightView = glm::lookAt(lightPos, center, up);

            // Build symmetric ortho projection (NO AABB)
            float extent = radius;
            float nearPlane = 0.0f;
            float farPlane  = radius * 2.0f;

            glm::mat4 lightProj = glm::ortho(
                -extent, extent,
                -extent, extent,
                nearPlane, farPlane
            );

            // Projection-space texel snapping
            glm::mat4 lightVP = lightProj * lightView;

            glm::vec4 origin = lightVP * glm::vec4(0, 0, 0, 1);
            origin /= origin.w;

            glm::vec2 shadowOrigin = glm::vec2(origin) * (shadowRes * 0.5f);
            glm::vec2 roundedOrigin = glm::round(shadowOrigin);
            glm::vec2 offset = (roundedOrigin - shadowOrigin) * (2.0f / shadowRes);

            lightProj[3][0] += offset.x;
            lightProj[3][1] += offset.y;

            // Final matrix
            return lightProj * lightView;
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