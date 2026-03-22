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

        if(m_Lights.at(index)->castShadow){
            Core::GetEngine().GetRenderer()->GetShadowManager()->RegisterLight(index, m_Lights[index]);
        }
        else{
            Core::GetEngine().GetRenderer()->GetShadowManager()->UnregisterLight(index);
        }
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