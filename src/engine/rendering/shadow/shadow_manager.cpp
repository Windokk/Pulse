#include "shadow_manager.hpp"

#include <iostream>

#include "engine/rendering/renderer/renderer.hpp"
#include "engine/ecs/components/rendering/camera.hpp"

#include "engine/core/engine.hpp"

namespace Pulse::Engine::Rendering{
    
    float ComputeCascadeSplitDistance(int cascadeIndex, float nearPlane, float farPlane, int totalCascades)
    {
        float nd = static_cast<float>(cascadeIndex + 1) / static_cast<float>(totalCascades);

        // Uniform split
        float logSplit = nearPlane * std::pow(farPlane / nearPlane, nd);

        float uniformSplit = nearPlane + (farPlane - nearPlane) * nd;

        float splitDist = 0.2f * logSplit + (1 - 0.2f) * uniformSplit;

        return splitDist;
    }

    /// @brief Initializer for the shadow manager
    /// @param resolution The resolution to use for shadow maps
    void ShadowManager::Init(int pointShadowsResolution, int spotShadowsResolution, int dirShadowsResolution)
    {
        OpenGL* gl = Core::GetEngine().GetGL();

        this->pointShadowsResolution = pointShadowsResolution;
        this->spotShadowsResolution = spotShadowsResolution;
        this->dirShadowsResolution = dirShadowsResolution;

        dirShader = Core::GetEngine().GetResourcesManager()->GetShader("shaders/shadows/shadow_dir");
        pointShader = Core::GetEngine().GetResourcesManager()->GetShader("shaders/shadows/shadow_point");
        spotShader = Core::GetEngine().GetResourcesManager()->GetShader("shaders/shadows/shadow_spot");
        shadowMaps.clear();
        pointLightCount = 0;
        currentCapacity = 0;
    }

    void ShadowManager::EnsureCubeArrayCapacity(int requiredPointLights)
    {
        if (requiredPointLights <= currentCapacity)
            return; // we already have enough layers

        OpenGL* gl = Core::GetEngine().GetGL();

        // Delete old array if it exists
        if (gl->IsTexture(cubeArrayTex))
            gl->DeleteTextures(1, &cubeArrayTex);

        currentCapacity = requiredPointLights;

        gl->GenTextures(1, &cubeArrayTex);
        gl->BindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, cubeArrayTex);
        gl->TexStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 1, GL_DEPTH_COMPONENT16,
            pointShadowsResolution, pointShadowsResolution, 6 * currentCapacity);
        gl->TexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        gl->TexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl->TexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl->TexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        gl->TexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        gl->TexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        gl->TexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    }

    /// @brief Register/Replace a light for shadow mappings
    /// @param lightIndex The updated/new light's global index in the scene
    /// @param light The point to the light's data
    void ShadowManager::RegisterLight(int lightIndex, std::shared_ptr<LightData> light)
    {
        OpenGL* gl = Core::GetEngine().GetGL();

        if (!light->castShadow)
            return;

        ShadowMap sm;
        sm.light = *light;

        if (light->type == int(LightType::Point)) {
            // --- Point Light (Cubemap Shadow) ---
            EnsureCubeArrayCapacity(pointLightCount + 1);

            sm.cubeArrayLayer = pointLightCount;
            
            sm.resolution = pointShadowsResolution;

            float near = 0.1f;
            float far = light->radius;
            glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, near, far);
            glm::vec3 pos = light->position;

            sm.shadowMatrices[0] = proj * glm::lookAt(pos, pos + glm::vec3( 1,  0,  0), glm::vec3(0, -1,  0));
            sm.shadowMatrices[1] = proj * glm::lookAt(pos, pos + glm::vec3(-1,  0,  0), glm::vec3(0, -1,  0));
            sm.shadowMatrices[2] = proj * glm::lookAt(pos, pos + glm::vec3( 0,  1,  0), glm::vec3(0,  0,  1));
            sm.shadowMatrices[3] = proj * glm::lookAt(pos, pos + glm::vec3( 0, -1,  0), glm::vec3(0,  0, -1));
            sm.shadowMatrices[4] = proj * glm::lookAt(pos, pos + glm::vec3( 0,  0,  1), glm::vec3(0, -1,  0));
            sm.shadowMatrices[5] = proj * glm::lookAt(pos, pos + glm::vec3( 0,  0, -1), glm::vec3(0, -1,  0));

        }
        else if (light->type == int(LightType::Directional)) {

            sm.resolution = dirShadowsResolution;

            // --- Directional Light (Cascaded Shadow Maps) ---
            for (int c = 0; c < CASCADES_PER_LIGHT; ++c) {
                gl->GenFramebuffers(1, &sm.fbo[c]);

                gl->GenTextures(1, &sm.depthMap[c]);
                gl->BindTexture(GL_TEXTURE_2D, sm.depthMap[c]);
                gl->TexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                        dirShadowsResolution, dirShadowsResolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
                gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
                gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
                gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

                gl->BindFramebuffer(GL_FRAMEBUFFER, sm.fbo[c]);
                gl->FramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, sm.depthMap[c], 0);
                gl->DrawBuffer(GL_NONE);
                gl->ReadBuffer(GL_NONE);
                gl->BindFramebuffer(GL_FRAMEBUFFER, 0);
            }
        }
        else if (light->type == int(LightType::Spot)) {

            sm.resolution = spotShadowsResolution;

            // --- Spot Light (Single 2D Shadow Map) ---
            gl->GenFramebuffers(1, &sm.fbo[0]);

            gl->GenTextures(1, &sm.depthMap[0]);
            gl->BindTexture(GL_TEXTURE_2D, sm.depthMap[0]);
            gl->TexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                        spotShadowsResolution, spotShadowsResolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
            gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
            gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
            gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

            gl->BindFramebuffer(GL_FRAMEBUFFER, sm.fbo[0]);
            gl->FramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, sm.depthMap[0], 0);
            gl->DrawBuffer(GL_NONE);
            gl->ReadBuffer(GL_NONE);
            gl->BindFramebuffer(GL_FRAMEBUFFER, 0);
            sm.lightMatrix[0] = light->GetLightMatrix();
        }


        // --- Replace or append the shadow map ---
        if (lightIndex < shadowMaps.size()) {
            ShadowMap& old = shadowMaps[lightIndex];

            // Clean up resources (if necessary)
            if (old.light.type == int(LightType::Directional)) {
                for (int c = 0; c < CASCADES_PER_LIGHT; ++c) {
                    if (gl->IsFramebuffer(old.fbo[c])) gl->DeleteFramebuffers(1, &old.fbo[c]);
                    if (gl->IsTexture(old.depthMap[c])) gl->DeleteTextures(1, &old.depthMap[c]);
                }
            }
            else if (old.light.type == int(LightType::Spot)) {
                if (gl->IsFramebuffer(old.fbo[0])) gl->DeleteFramebuffers(1, &old.fbo[0]);
                if (gl->IsTexture(old.depthMap[0])) gl->DeleteTextures(1, &old.depthMap[0]);
            }

            shadowMaps[lightIndex] = sm;
        } else {
            shadowMaps.push_back(sm);
            if (sm.light.type == int(LightType::Point))
                ++pointLightCount;
        }
    }

    void ShadowManager::TryShrinkCubeArray()
    {
        OpenGL* gl = Core::GetEngine().GetGL();

        // Only shrink if capacity is significantly larger than usage
        if (pointLightCount >= currentCapacity / 2 || currentCapacity <= 4)
            return;

        // Delete old array
        if (gl->IsTexture(cubeArrayTex))
            gl->DeleteTextures(1, &cubeArrayTex);

        currentCapacity = std::max(4, pointLightCount); // never shrink below 4 layers

        gl->GenTextures(1, &cubeArrayTex);
        gl->BindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, cubeArrayTex);
        gl->TexStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 1, GL_DEPTH_COMPONENT16,
            pointShadowsResolution, pointShadowsResolution, 6 * currentCapacity);

        // reapply texture parameters
        gl->TexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        gl->TexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl->TexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl->TexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        gl->TexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        gl->TexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        gl->TexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    }

    /// @brief Remove a light from shadow mappings
    /// @param lightIndex The light's global index in the scene
    void ShadowManager::UnregisterLight(int lightIndex)
    {
        OpenGL* gl = Core::GetEngine().GetGL();

        if (lightIndex < 0 || lightIndex >= static_cast<int>(shadowMaps.size()))
            return;

        ShadowMap& removedSM = shadowMaps[lightIndex];

        // ---- Clean up Directional Light Resources (multiple cascades) ----
        if (removedSM.light.type == static_cast<int>(LightType::Directional)) {
            for (int c = 0; c < CASCADES_PER_LIGHT; ++c) {
                if (gl->IsFramebuffer(removedSM.fbo[c]))
                    gl->DeleteFramebuffers(1, &removedSM.fbo[c]);

                if (gl->IsTexture(removedSM.depthMap[c]))
                    gl->DeleteTextures(1, &removedSM.depthMap[c]);
            }
        }

        // ---- Clean up Spot Light Resources (single map) ----
        else if (removedSM.light.type == static_cast<int>(LightType::Spot)) {
            if (gl->IsFramebuffer(removedSM.fbo[0]))
                gl->DeleteFramebuffers(1, &removedSM.fbo[0]);

            if (gl->IsTexture(removedSM.depthMap[0]))
                gl->DeleteTextures(1, &removedSM.depthMap[0]);
        }

        // ---- Special Case: Point Light (cube map layer) ----
        else if (removedSM.light.type == static_cast<int>(LightType::Point)) {
            int removedLayer = removedSM.cubeArrayLayer;

            // Shift down cube array layer indices for other point lights
            for (auto& sm : shadowMaps) {
                if (sm.light.type == static_cast<int>(LightType::Point) &&
                    sm.cubeArrayLayer > removedLayer) {
                    --sm.cubeArrayLayer;
                }
            }

            --pointLightCount;

            TryShrinkCubeArray();
        }

        // ---- Remove from list ----
        shadowMaps.erase(shadowMaps.begin() + lightIndex);
    }

    /// @brief Render the scene into each shadow map
    /// @param meshes The meshes to render. All other meshes will be occluded from the shadow pass
    void ShadowManager::RenderShadowMaps(const std::vector<std::pair<glm::mat4, Rendering::Mesh*>> &meshes, std::shared_ptr<ECS::Components::Camera> cam)
    {
        OpenGL* gl = Core::GetEngine().GetGL();

        gl->Enable(GL_DEPTH_TEST);
        gl->Enable(GL_CULL_FACE);
        gl->CullFace(GL_FRONT);
        gl->FrontFace(GL_CW);

        for (auto& sm : shadowMaps)
        {
            LightData* light = &sm.light;

            if (!light->castShadow)
                continue;

            if (light->type == static_cast<int>(LightType::Point))
            {
                gl->Viewport(0, 0, sm.resolution, sm.resolution);
                gl->BindFramebuffer(GL_FRAMEBUFFER, sm.fbo[0]);  // Reused FBO

                pointShader->Activate();

                for (int face = 0; face < 6; ++face)
                {
                    // Attach face of cubemap array for current point light layer
                    gl->FramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                            cubeArrayTex, 0, sm.cubeArrayLayer * 6 + face);

                    gl->Clear(GL_DEPTH_BUFFER_BIT);

                    pointShader->setMat4("shadowMatrices[0]", sm.shadowMatrices[face]);
                    pointShader->setFloat("farPlane", light->radius);
                    pointShader->setVec3("lightPos", light->position);

                    for (const auto& pair : meshes)
                    {
                        pointShader->setMat4("model", pair.first);
                        pair.second->DrawWithoutMaterial();
                    }
                }

                pointShader->Deactivate();
                gl->BindFramebuffer(GL_FRAMEBUFFER, 0);
            }
            else if (light->type == static_cast<int>(LightType::Spot))
            {
                gl->Viewport(0, 0, sm.resolution, sm.resolution);
                gl->BindFramebuffer(GL_FRAMEBUFFER, sm.fbo[0]);
                gl->Clear(GL_DEPTH_BUFFER_BIT);

                spotShader->Activate();
                
                spotShader->setMat4("lightSpaceMatrix", sm.lightMatrix[0]);

                for (const auto& pair : meshes)
                {
                    spotShader->setMat4("model", pair.first);
                    pair.second->DrawWithoutMaterial();
                }

                spotShader->Deactivate();
                gl->BindFramebuffer(GL_FRAMEBUFFER, 0);
            }
            else if (light->type == static_cast<int>(LightType::Directional))
            {
                // Render each cascade individually
                dirShader->Activate();

                 for (int c = 0; c < CASCADES_PER_LIGHT; ++c)
                    sm.cascadeSplits[c] = ComputeCascadeSplitDistance(c, cam->nearPlane, cam->farPlane, CASCADES_PER_LIGHT);

                for (int c = 0; c < CASCADES_PER_LIGHT; ++c)
                {
                    gl->Viewport(0, 0, sm.resolution, sm.resolution);
                    gl->BindFramebuffer(GL_FRAMEBUFFER, sm.fbo[c]);
                    gl->Clear(GL_DEPTH_BUFFER_BIT);

                    float splitNear = c == 0 ? cam->nearPlane : sm.cascadeSplits[c - 1];
                    float splitFar  = sm.cascadeSplits[c];
                    sm.lightMatrix[c] = light->GetLightMatrix(cam->GetView(), cam->fov, cam->GetSize().x/cam->GetSize().y, splitNear, splitFar, dirShadowsResolution);

                    dirShader->setMat4("lightSpaceMatrix", sm.lightMatrix[c]);

                    for (const auto& pair : meshes)
                    {
                        dirShader->setMat4("model", pair.first);
                        pair.second->DrawWithoutMaterial();
                    }

                    gl->BindFramebuffer(GL_FRAMEBUFFER, 0);
                }

                dirShader->Deactivate();
            }
        }

        gl->Disable(GL_CULL_FACE);
        gl->Disable(GL_DEPTH_TEST);
    }

    /// @brief Bind the shadow maps to a material
    /// @param material The material to bind shadow maps to
    void ShadowManager::BindShadowMaps(std::shared_ptr<Pulse::Engine::Rendering::Material> material)
    {
        OpenGL* gl = Core::GetEngine().GetGL();

        constexpr int MAX_SHADOW_LIGHTS = 16;
        constexpr int MAX_SPOT_LIGHTS = 12;
        constexpr int MAX_POINT_LIGHTS = 12;

        int textureUnit = 10;  // Start binding textures from unit 10

        // Initialize all shadow map references and matrices to safe defaults
        for (int i = 0; i < NUM_CASCADES; ++i) {
            material->SetScalarParameter("shadow_dirShadowMaps[" + std::to_string(i) + "]", 0);
            material->SetScalarParameter("shadow_dirLightSpaceMatrices[" + std::to_string(i) + "]", glm::mat4(1.0f));
            material->SetScalarParameter("cascadeSplits[" + std::to_string(i) + "]", 1.0f); // Default to max depth
        }

        for (int i = 0; i < MAX_SPOT_LIGHTS; ++i) {
            material->SetScalarParameter("shadow_spotShadowMaps[" + std::to_string(i) + "]", 0);
            material->SetScalarParameter("shadow_spotLightSpaceMatrices[" + std::to_string(i) + "]", glm::mat4(1.0f));
        }

        for (int i = 0; i < MAX_POINT_LIGHTS; ++i) {
            material->SetScalarParameter("shadow_pointLightFarPlanes[" + std::to_string(i) + "]", 0.0f);
        }

        int cascadeIndex = 0;
        int spotIndex = 0;

        for (const auto& sm : shadowMaps)
        {
            const LightData& light = sm.light;

            if (!light.castShadow)
                continue;

            // Handle Directional Lights (with cascades)
            if (light.type == static_cast<int>(LightType::Directional))
            {
                for (int c = 0; c < CASCADES_PER_LIGHT; ++c)
                {
                    if (cascadeIndex >= NUM_CASCADES || textureUnit >= 32)
                        break;

                    gl->ActiveTexture(GL_TEXTURE0 + textureUnit);
                    gl->BindTexture(GL_TEXTURE_2D, sm.depthMap[c]);

                    material->SetScalarParameter("shadow_dirShadowMaps[" + std::to_string(cascadeIndex) + "]", textureUnit);
                    material->SetScalarParameter("shadow_dirLightSpaceMatrices[" + std::to_string(cascadeIndex) + "]", sm.lightMatrix[c]);
                    material->SetScalarParameter("cascadeSplits[" + std::to_string(cascadeIndex) + "]", sm.cascadeSplits[c]);
                    
                    ++cascadeIndex;
                    ++textureUnit;
                }
            }
            // Spot lights
            else if (light.type == static_cast<int>(LightType::Spot))
            {
                if (spotIndex >= MAX_SPOT_LIGHTS || textureUnit >= 32)
                    break;

                gl->ActiveTexture(GL_TEXTURE0 + textureUnit);
                gl->BindTexture(GL_TEXTURE_2D, sm.depthMap[0]);

                material->SetScalarParameter("shadow_spotShadowMaps[" + std::to_string(spotIndex) + "]", textureUnit);
                material->SetScalarParameter("shadow_spotLightSpaceMatrices[" + std::to_string(spotIndex) + "]", sm.lightMatrix[0]);

                ++spotIndex;
                ++textureUnit;
            }
            // Point lights
            else if (light.type == static_cast<int>(LightType::Point))
            {
                if (sm.cubeArrayLayer >= MAX_POINT_LIGHTS)
                    continue;

                material->SetScalarParameter("shadow_pointLightFarPlanes[" + std::to_string(sm.cubeArrayLayer) + "]", light.radius);
            }
        }

        // Bind cube map array texture (point lights)
        material->SetTextureParameter("shadow_pointShadowMapArray", cubeArrayTex);
    }
}