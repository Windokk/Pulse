#include "shadow_manager.hpp"

#include "engine/ecs/components/rendering/camera.hpp"

#include "engine/rendering/shader/shader.hpp"
#include "engine/rendering/material/material.hpp"
#include "engine/rendering/mesh/mesh.hpp"
#include "engine/rendering/texture/texture.hpp"
#include "engine/rendering/texture/cubemap/cubemap.hpp"
#include "engine/rendering/pipeline/pipeline.hpp"
#include "engine/rendering/framebuffer/framebuffer.hpp"

#include "engine/core/engine.hpp"

#include "engine/rendering/camera/camera_manager.hpp"

#include "engine/core/resources/resources_manager.hpp"

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

    void ShadowManager::Init(int pointShadowsResolution, int spotShadowsResolution, int dirShadowsResolution)
    {
        m_PointShadowsResolution = pointShadowsResolution;
        m_SpotShadowsResolution = spotShadowsResolution;
        m_DirShadowsResolution = dirShadowsResolution;

        m_DirShader = Core::GetEngine().GetResourcesManager()->GetShader("shaders/shadows/shadow_dir");
        m_PointShader = Core::GetEngine().GetResourcesManager()->GetShader("shaders/shadows/shadow_point");
        m_SpotShader = Core::GetEngine().GetResourcesManager()->GetShader("shaders/shadows/shadow_spot");
        m_ShadowMaps.clear();
        m_PointLightCount = 0;
        m_CurrentPointLightCapacity = 0;

        EnsureCubeArrayCapacity(0);
    }

    void ShadowManager::UpdatePassUniforms()
    {
        std::shared_ptr<ECS::Components::Camera> cam = Core::GetEngine().GetCameraManager()->GetActiveCamera();

        for(auto& sm : m_ShadowMaps){
            if(sm.second.light.type == static_cast<int>(LightType::Directional))
            {
                for (int c = 0; c < CASCADES_PER_LIGHT; ++c)
                {
                    auto pass = Core::GetEngine().GetRenderer()->GetRenderPass(sm.second.passes[c]);

                    float splitNear = c == 0 ? cam->nearPlane : sm.second.cascadeSplits[c - 1];
                    float splitFar  = sm.second.cascadeSplits[c];
                    sm.second.lightMatrix[c] = sm.second.light.GetLightMatrix(cam->GetView(), *cam->GetFOV(), cam->GetSize().x/cam->GetSize().y, splitNear, splitFar, m_DirShadowsResolution);

                    pass->customUniforms["lightSpaceMatrix"] = sm.second.lightMatrix[c];
                }
            }
        }
    }

    void ShadowManager::SubmitPasses(int lightIndex)
    {
        if (m_ShadowMaps.find(lightIndex) == m_ShadowMaps.end()){
            DEBUG_ERROR("Tried submitting shadow passes for a light, but this light was never registered.");
            return;
        }

        std::vector<std::shared_ptr<RenderPass>> passes = {};

        std::shared_ptr<ECS::Components::Camera> cam = Core::GetEngine().GetCameraManager()->GetActiveCamera();

        ShadowMap* sm = &m_ShadowMaps.at(lightIndex);
        LightData light = sm->light;

        if (light.type == static_cast<int>(LightType::Point))
        {
            PipelineSpecifications pointSpecs;
            pointSpecs.blending = false;
            pointSpecs.cullMode = CullMode::None;
            pointSpecs.depthTest = true;
            pointSpecs.polygonMode = PolygonMode::Fill;
            pointSpecs.topology = PrimitiveTopology::Triangles;
            pointSpecs.shader = m_PointShader;
            pointSpecs.vertexLayout = {{{"aPos", ShaderDataType::Vec3, 0}}, sizeof(glm::vec3)};
            std::shared_ptr<Pipeline> pointShadowPassPipeline = Core::GetEngine().GetRenderer()->GetOrAdd(pointSpecs);
            
            std::shared_ptr<RenderPass> pass = std::make_shared<RenderPass>();
            pass->target = sm->framebuffer[0];
            pass->clearColor = false;
            pass->clearDepth = true;
            pass->customPipeline = pointShadowPassPipeline;
            pass->overridePipeline = true;

            for(int i = 0; i < 6; i++)
                pass->customUniforms.emplace("shadowMatrices[" + std::to_string(i) + "]", sm->shadowMatrices[i]);

            pass->customUniforms.emplace("farPlane", light.radius);
            pass->customUniforms.emplace("lightPos", light.position);
            pass->customUniforms.emplace("cubeIndex", sm->cubeArrayLayer);

            pass->allowResize = false;
            pass->allowCulling = false;

            std::string passName = "ShadowPass_" + std::to_string(lightIndex) + "_" + std::to_string(passes.size());
            sm->passes.push_back(passName);

            passes.push_back(pass);
        }
        else if(light.type == static_cast<int>(LightType::Spot))
        {
            PipelineSpecifications spotSpecs;
            spotSpecs.blending = false;
            spotSpecs.cullMode = CullMode::None;
            spotSpecs.depthTest = true;
            spotSpecs.polygonMode = PolygonMode::Fill;
            spotSpecs.topology = PrimitiveTopology::Triangles;
            spotSpecs.shader = m_SpotShader;
            spotSpecs.vertexLayout = {{{"aPos", ShaderDataType::Vec3, 0}}, sizeof(glm::vec3)};
            std::shared_ptr<Pipeline> spotShadowPassPipeline = Core::GetEngine().GetRenderer()->GetOrAdd(spotSpecs);

            std::shared_ptr<RenderPass> pass = std::make_shared<RenderPass>();
            pass->target = sm->framebuffer[0];
            pass->clearColor = false;
            pass->clearDepth = true;
            pass->customPipeline = spotShadowPassPipeline;
            pass->overridePipeline = true;
            pass->customUniforms.emplace("lightSpaceMatrix", sm->lightMatrix[0]);
            pass->allowResize = false;
            pass->allowCulling = false;
            
            std::string passName = "ShadowPass_" + std::to_string(lightIndex) + "_" + std::to_string(passes.size());
            
            passes.push_back(pass);
        }
        else if(light.type == static_cast<int>(LightType::Directional))
        {
            PipelineSpecifications dirSpecs;
            dirSpecs.blending = false;
            dirSpecs.cullMode = CullMode::None;
            dirSpecs.depthTest = true;
            dirSpecs.polygonMode = PolygonMode::Fill;
            dirSpecs.topology = PrimitiveTopology::Triangles;
            dirSpecs.shader = m_DirShader;
            dirSpecs.vertexLayout = {{{"aPos", ShaderDataType::Vec3, 0}}, sizeof(glm::vec3)};

            std::shared_ptr<Pipeline> dirShadowPassPipeline = Core::GetEngine().GetRenderer()->GetOrAdd(dirSpecs);

            for (int c = 0; c < CASCADES_PER_LIGHT; ++c)
                sm->cascadeSplits[c] = ComputeCascadeSplitDistance(c, cam->nearPlane, cam->farPlane, CASCADES_PER_LIGHT);

            for (int c = 0; c < CASCADES_PER_LIGHT; ++c)
            {
                std::shared_ptr<RenderPass> pass = std::make_shared<RenderPass>();
                pass->target = sm->framebuffer[c];
                pass->clearColor = false;
                pass->clearDepth = true;
                pass->customPipeline = dirShadowPassPipeline;
                pass->overridePipeline = true;

                float splitNear = c == 0 ? cam->nearPlane : sm->cascadeSplits[c - 1];
                float splitFar  = sm->cascadeSplits[c];
                sm->lightMatrix[c] = light.GetLightMatrix(cam->GetView(), *cam->GetFOV(), cam->GetSize().x/cam->GetSize().y, splitNear, splitFar, m_DirShadowsResolution);

                pass->customUniforms.emplace("lightSpaceMatrix", sm->lightMatrix[c]);

                pass->allowResize = false;
                pass->allowCulling = false;
                
                std::string passName = "ShadowPass_" + std::to_string(lightIndex) + "_" + std::to_string(passes.size());
                sm->passes.push_back(passName); 

                passes.push_back(pass);
            }
        }

        for(int i = 0; i < passes.size(); i++){
            std::string passName = "ShadowPass_" + std::to_string(lightIndex) + "_" + std::to_string(i);

            Core::GetEngine().GetRenderer()->AddRenderPass(passes[i], passName, {});
        }
    }

    void ShadowManager::EnsureCubeArrayCapacity(int requiredPointLights)
    {
        if (requiredPointLights <= m_CurrentPointLightCapacity)
            return; // we already have enough layers

        // Delete old array if it exists
        if(m_CubeArrayTex){
            if (m_CubeArrayTex->IsValid())
               m_CubeArrayTex.reset();
        }

        m_CurrentPointLightCapacity = requiredPointLights;

        TextureSpecifications texSpecs;

        texSpecs.internalFormat = TextureInternalFormat::Depth16;
        texSpecs.compareMode = TextureCompareMode::CompareRefToTexture;
        texSpecs.compareFunc = TextureCompareFunc::LessOrEqual;
        texSpecs.width = m_PointShadowsResolution;
        texSpecs.height = m_PointShadowsResolution;
        texSpecs.minFilter = TextureFilter::Linear;
        texSpecs.magFilter = TextureFilter::Linear;
        texSpecs.wrapR = TextureWrap::ClampEdge;
        texSpecs.wrapS = TextureWrap::ClampEdge;
        texSpecs.wrapT = TextureWrap::ClampEdge;

        std::vector<std::array<unsigned char *, 6>> emptyData;
        for(int i = 0; i < m_CurrentPointLightCapacity; i++){
            emptyData.push_back({nullptr, nullptr, nullptr, nullptr, nullptr, nullptr});
        }

        m_CubeArrayTex = CubemapArray::Create(texSpecs, emptyData);
    }

    void ShadowManager::RegisterLight(int lightIndex, std::shared_ptr<LightData> light)
    {
        if (!light->castShadow){
            if(m_ShadowMaps.find(lightIndex) != m_ShadowMaps.end()){
                UnregisterLight(lightIndex);
            }
            return;
        }

        if (m_ShadowMaps.find(lightIndex) != m_ShadowMaps.end())
        {
            
            ShadowMap* oldSM = &m_ShadowMaps.at(lightIndex);

            // ---- Clean up Directional Light Resources (multiple cascades) ----
            if (oldSM->light.type == static_cast<int>(LightType::Directional)) {
                for (int c = 0; c < CASCADES_PER_LIGHT; ++c) {
                    if (oldSM->framebuffer[c]->IsValid())
                        oldSM->framebuffer[c]->Destroy();
                }
            }

            // ---- Clean up Spot Light Resources (single map) ----
            else if (oldSM->light.type == static_cast<int>(LightType::Spot)) {
                if (oldSM->framebuffer[0]->IsValid())
                        oldSM->framebuffer[0]->Destroy();
            }

            // ---- Special Case: Point Light (cube map layer) ----
            else if (oldSM->light.type == static_cast<int>(LightType::Point)) {
                int removedLayer = oldSM->cubeArrayLayer;
                // Shift down cube array layer indices for other point lights
                for (auto& oldSM : m_ShadowMaps) {
                    if (oldSM.second.light.type == static_cast<int>(LightType::Point) &&
                        oldSM.second.cubeArrayLayer > removedLayer) {
                        --oldSM.second.cubeArrayLayer;
                    }
                }

                --m_PointLightCount;

                TryShrinkCubeArray();
            }
        }

        ShadowMap sm;
        sm.light = *light;

        if (light->type == int(LightType::Point)) {
            // --- Point Light (Cubemap Shadow) ---
            EnsureCubeArrayCapacity(m_PointLightCount + 1);

            sm.cubeArrayLayer = m_PointLightCount;
            
            sm.resolution = m_PointShadowsResolution;

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

            FramebufferSpecifications specs;
            specs.hasColor = false;
            specs.hasDepth = true;
            specs.multisampled = false;
            specs.width = m_PointShadowsResolution;
            specs.height = m_PointShadowsResolution;
            specs.depthSpecs.internalFormat = TextureInternalFormat::Depth32F;

            sm.framebuffer[0] = Framebuffer::Create(specs);
        }
        else if (light->type == int(LightType::Directional)) {

            sm.resolution = m_DirShadowsResolution;

            // --- Directional Light (Cascaded Shadow Maps) ---
            for (int c = 0; c < CASCADES_PER_LIGHT; ++c) {

                TextureSpecifications textureSpecs;
                textureSpecs.borderColor = COL_RGBA(1.0f);
                textureSpecs.compareFunc = TextureCompareFunc::LessOrEqual;
                textureSpecs.compareMode = TextureCompareMode::CompareRefToTexture;
                textureSpecs.minFilter = TextureFilter::Linear;
                textureSpecs.magFilter = TextureFilter::Linear;
                textureSpecs.wrapS = TextureWrap::ClampBorder;
                textureSpecs.wrapT = TextureWrap::ClampBorder;
                textureSpecs.wrapR = TextureWrap::ClampBorder;
                textureSpecs.internalFormat = TextureInternalFormat::Depth32F;
                textureSpecs.format = TextureFormat::Depth;

                FramebufferSpecifications specs;
                specs.hasColor = false;
                specs.hasDepth = true;
                specs.multisampled = false;
                specs.width = m_DirShadowsResolution;
                specs.height = m_DirShadowsResolution;
                specs.depthSpecs = textureSpecs;

                sm.framebuffer[c] = Framebuffer::Create(specs);
            }
        }
        else if (light->type == int(LightType::Spot)) {

            sm.resolution = m_SpotShadowsResolution;

            // --- Spot Light (Single 2D Shadow Map) ---
            TextureSpecifications textureSpecs;
            textureSpecs.borderColor = COL_RGBA(1.0f);
            textureSpecs.compareFunc = TextureCompareFunc::LessOrEqual;
            textureSpecs.compareMode = TextureCompareMode::CompareRefToTexture;
            textureSpecs.minFilter = TextureFilter::Linear;
            textureSpecs.magFilter = TextureFilter::Linear;
            textureSpecs.wrapS = TextureWrap::ClampBorder;
            textureSpecs.wrapT = TextureWrap::ClampBorder;
            textureSpecs.internalFormat = TextureInternalFormat::Depth32F;
            textureSpecs.format = TextureFormat::Depth;

            FramebufferSpecifications specs;
            specs.hasColor = false;
            specs.hasDepth = true;
            specs.multisampled = false;
            specs.width = m_SpotShadowsResolution;
            specs.height = m_SpotShadowsResolution;
            specs.depthSpecs = textureSpecs;

            sm.framebuffer[0] = Framebuffer::Create(specs);

            sm.lightMatrix[0] = light->GetLightMatrix();
        }


        m_ShadowMaps[lightIndex] = sm;
        if (sm.light.type == int(LightType::Point))
            ++m_PointLightCount;
        
        SubmitPasses(lightIndex);
    }

    void ShadowManager::TryShrinkCubeArray()
    {
        // Only shrink if capacity is significantly larger than usage
        if (m_PointLightCount >= m_CurrentPointLightCapacity / 2 || m_CurrentPointLightCapacity <= 4)
            return;

        // Delete old array
        if(m_CubeArrayTex){
            if(m_CubeArrayTex->IsValid())
                m_CubeArrayTex.reset();
        }

        m_CurrentPointLightCapacity = std::max(4, m_PointLightCount); // never shrink below 4 layers

        TextureSpecifications texSpecs;

        texSpecs.internalFormat = TextureInternalFormat::Depth16;
        texSpecs.format = TextureFormat::Depth;
        texSpecs.compareMode = TextureCompareMode::CompareRefToTexture;
        texSpecs.compareFunc = TextureCompareFunc::LessOrEqual;
        texSpecs.width = m_PointShadowsResolution;
        texSpecs.height = m_PointShadowsResolution;
        texSpecs.minFilter = TextureFilter::Linear;
        texSpecs.magFilter = TextureFilter::Linear;
        texSpecs.wrapR = TextureWrap::ClampEdge;
        texSpecs.wrapS = TextureWrap::ClampEdge;
        texSpecs.wrapT = TextureWrap::ClampEdge;

        std::vector<std::array<unsigned char *, 6>> emptyData;
        for(int i = 0; i < m_CurrentPointLightCapacity; i++){
            emptyData.push_back({nullptr, nullptr, nullptr, nullptr, nullptr, nullptr});
        }
        m_CubeArrayTex = CubemapArray::Create(texSpecs, emptyData);
    }

    void ShadowManager::UnregisterLight(int lightIndex)
    {
        if (m_ShadowMaps.find(lightIndex) == m_ShadowMaps.end()){
            DEBUG_ERROR("Tried unregistering a light from shadow mappings, but this light was never registered.");
            return;
        }

        ShadowMap& removedSM = m_ShadowMaps[lightIndex];

        // ---- Clean up Directional Light Resources (multiple cascades) ----
        if (removedSM.light.type == static_cast<int>(LightType::Directional)) {
            for (int c = 0; c < CASCADES_PER_LIGHT; ++c) {
                if (removedSM.framebuffer[c]->IsValid()){
                    removedSM.framebuffer[c]->Destroy();
                    removedSM.framebuffer[c].reset();
                }
            }
        }

        // ---- Clean up Spot Light Resources (single map) ----
        else if (removedSM.light.type == static_cast<int>(LightType::Spot)) {
            if (removedSM.framebuffer[0]->IsValid()){
                removedSM.framebuffer[0]->Destroy();
                removedSM.framebuffer[0].reset();
            }
        }

        // ---- Special Case: Point Light (cube map layer) ----
        else if (removedSM.light.type == static_cast<int>(LightType::Point)) {
            int removedLayer = removedSM.cubeArrayLayer;

            // Shift down cube array layer indices for other point lights
            for (auto& sm : m_ShadowMaps) {
                if (sm.second.light.type == static_cast<int>(LightType::Point) &&
                    sm.second.cubeArrayLayer > removedLayer) {
                    --sm.second.cubeArrayLayer;
                }
            }

            --m_PointLightCount;

            TryShrinkCubeArray();
        }

        for(std::string passName : removedSM.passes){
            Core::GetEngine().GetRenderer()->RemoveRenderPass(passName);
        }

        // ---- Remove from list ----
        m_ShadowMaps.erase(lightIndex);
    }

    void ShadowManager::BindShadowMaps(std::shared_ptr<Pulse::Engine::Rendering::Material> material)
    {
        constexpr int MAX_SPOT_LIGHTS  = 10;
        constexpr int MAX_POINT_LIGHTS = 10;

        // Bind shadow textures
        int spotIndex    = 0;

        for (const auto& sm : m_ShadowMaps)
        {
            const LightData& light = sm.second.light;

            if (!light.castShadow)
                continue;

            // Directional lights
            int dirCascadeIndex = 0;
            if (light.type == Directional)
            {
                for (int c = 0; c < CASCADES_PER_LIGHT; ++c)
                {
                    material->SetTextureParameter(
                        "shadow_dirShadowMaps[" + std::to_string(dirCascadeIndex) + "]",
                        sm.second.framebuffer[c]->GetDepthAttachment()
                    );

                    material->SetScalarParameter(
                        "shadow_dirLightSpaceMatrices[" + std::to_string(dirCascadeIndex) + "]",
                        sm.second.lightMatrix[c]
                    );

                    material->SetScalarParameter(
                        "shadow_cascadeSplits[" + std::to_string(dirCascadeIndex) + "]",
                        sm.second.cascadeSplits[c]
                    );

                    dirCascadeIndex++;
                }
            }
            // Spot lights
            else if (light.type == static_cast<int>(LightType::Spot))
            {
                if (spotIndex >= MAX_SPOT_LIGHTS)
                    continue;

                // Set sampler uniform
                material->SetTextureParameter(
                    "shadow_spotShadowMaps[" + std::to_string(spotIndex) + "]",
                    sm.second.framebuffer[0]->GetDepthAttachment()
                );

                material->SetScalarParameter(
                    "shadow_spotLightSpaceMatrices[" + std::to_string(spotIndex) + "]",
                    sm.second.lightMatrix[0]
                );

                ++spotIndex;
            }
            // Point lights
            else if (light.type == static_cast<int>(LightType::Point))
            {
                if (sm.second.cubeArrayLayer >= MAX_POINT_LIGHTS)
                    continue;

                material->SetScalarParameter(
                    "shadow_pointLightFarPlanes[" + std::to_string(sm.second.cubeArrayLayer) + "]",
                    light.radius
                );
            }
        }

        // Bind point light cube map array
        if(m_CurrentPointLightCapacity)
            material->SetTextureParameter("shadow_pointShadowMapArray", m_CubeArrayTex->GetHandle());
    }

    void ShadowManager::ClearAll()
    {
        m_ShadowMaps.clear();
    }
}