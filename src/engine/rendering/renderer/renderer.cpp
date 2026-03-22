#include "renderer.hpp"

#include "engine/core/engine.hpp"

#include "engine/rendering/material/material.hpp"
#include "engine/rendering/pipeline/pipeline.hpp"
#include "engine/rendering/shader/shader.hpp"
#include "engine/rendering/mesh/mesh.hpp"
#include "engine/rendering/lighting/shadow_manager.hpp"
#include "engine/rendering/camera/camera_manager.hpp"
#include <queue>
#include <glm/gtx/string_cast.hpp>

namespace Pulse::Engine::Rendering{

    void Renderer::Init(std::shared_ptr<RendererSettings> initialSettings)
    {
        m_Settings = initialSettings;
        m_RendererAPI = RendererAPI::Create(initialSettings->api);

        ToggleMultisampling(initialSettings->multisampling);

        //Init viewport framebuffers
        FramebufferSpecifications viewportSpecs;
        viewportSpecs.width = m_Settings->viewportWidth;
        viewportSpecs.height = m_Settings->viewportHeight;
        viewportSpecs.hasColor = true;
        viewportSpecs.hasDepth = true;
        viewportSpecs.multisampled = true;
        m_ViewportBuffer = Framebuffer::Create(viewportSpecs);

        //Init base geometry
        /// Unit cube
        
        struct Vertex {
            glm::vec3 position;
            glm::vec2 texCoord;
        };

        VertexLayout basicVertexLayout{
            {{"aPos",       ShaderDataType::Vec3, 0, offsetof(Vertex, position)},
            {"aTexCoord", ShaderDataType::Vec2, 1, offsetof(Vertex, texCoord)}},
            sizeof(Vertex)
        };
        std::vector<Vertex> cubeVertices = {
            // Front
            {{-0.5f,-0.5f, 0.5f}, {0.0f,0.0f}},
            {{ 0.5f,-0.5f, 0.5f}, {1.0f,0.0f}},
            {{ 0.5f, 0.5f, 0.5f}, {1.0f,1.0f}},
            {{-0.5f, 0.5f, 0.5f}, {0.0f,1.0f}},

            // Back
            {{ 0.5f,-0.5f,-0.5f}, {0.0f,0.0f}},
            {{-0.5f,-0.5f,-0.5f}, {1.0f,0.0f}},
            {{-0.5f, 0.5f,-0.5f}, {1.0f,1.0f}},
            {{ 0.5f, 0.5f,-0.5f}, {0.0f,1.0f}},

            // Left
            {{-0.5f,-0.5f,-0.5f}, {0.0f,0.0f}},
            {{-0.5f,-0.5f, 0.5f}, {1.0f,0.0f}},
            {{-0.5f, 0.5f, 0.5f}, {1.0f,1.0f}},
            {{-0.5f, 0.5f,-0.5f}, {0.0f,1.0f}},

            // Right
            {{ 0.5f,-0.5f, 0.5f}, {0.0f,0.0f}},
            {{ 0.5f,-0.5f,-0.5f}, {1.0f,0.0f}},
            {{ 0.5f, 0.5f,-0.5f}, {1.0f,1.0f}},
            {{ 0.5f, 0.5f, 0.5f}, {0.0f,1.0f}},

            // Top
            {{-0.5f, 0.5f, 0.5f}, {0.0f,0.0f}},
            {{ 0.5f, 0.5f, 0.5f}, {1.0f,0.0f}},
            {{ 0.5f, 0.5f,-0.5f}, {1.0f,1.0f}},
            {{-0.5f, 0.5f,-0.5f}, {0.0f,1.0f}},

            // Bottom
            {{-0.5f,-0.5f,-0.5f}, {0.0f,0.0f}},
            {{ 0.5f,-0.5f,-0.5f}, {1.0f,0.0f}},
            {{ 0.5f,-0.5f, 0.5f}, {1.0f,1.0f}},
            {{-0.5f,-0.5f, 0.5f}, {0.0f,1.0f}}
        };
        std::vector<uint32_t> cubeIndices = {
            0,1,2, 2,3,0,       // Front
            4,5,6, 6,7,4,       // Back
            8,9,10, 10,11,8,    // Left
            12,13,14, 14,15,12, // Right
            16,17,18, 18,19,16, // Top
            20,21,22, 22,23,20  // Bottom
        };
        m_UnitCube = Mesh::Create();
        std::vector<uint8_t> cubeVertexBuffer;
        cubeVertexBuffer.resize(cubeVertices.size() * sizeof(Vertex));
        memcpy(cubeVertexBuffer.data(), cubeVertices.data(), cubeVertexBuffer.size());
        m_UnitCube->Create(cubeVertexBuffer, cubeIndices, basicVertexLayout);

        /// Unit quad
        std::vector<Vertex> quadVertices = {
            {{-1.0f,  1.0f, 0.0f}, {0.0f, 1.0f}}, // top-left
            {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}}, // bottom-left
            {{ 1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}}, // bottom-right
            {{ 1.0f,  1.0f, 0.0f}, {1.0f, 1.0f}}  // top-right
        };
        std::vector<uint32_t> quadIndices = {
            0, 1, 2,
            2, 3, 0
        };
        m_UnitQuad = Mesh::Create();
        std::vector<uint8_t> quadVertexBuffer;
        quadVertexBuffer.resize(quadVertices.size() * sizeof(Vertex));
        memcpy(quadVertexBuffer.data(), quadVertices.data(), quadVertexBuffer.size());
        m_UnitQuad->Create(quadVertexBuffer, quadIndices, basicVertexLayout);

        // Init lighting
        m_LightManager = std::make_shared<LightManager>();
        m_ShadowManager = std::make_shared<ShadowManager>();
        m_ShadowManager->Init(512, 1024, 2048);

        //Init built-in passes
        /// Forward pass
        std::shared_ptr<RenderPass> forwardPass = std::make_shared<RenderPass>();
        forwardPass->target = m_ViewportBuffer;
        forwardPass->clearColor = true;
        forwardPass->clearDepth = true;
        forwardPass->overridePipeline = false;
        AddRenderPass(forwardPass, "ForwardPass", {"ShadowPass0", "ShadowPass1", "ShadowPass2"});
        m_RendererAPI->SetClearColor(0,0,0,1);
    }

    void Renderer::BuildExecutionOrder()
    {
        m_ExecutionOrder.clear();

        std::unordered_map<std::string, int> inDegree;
        std::unordered_map<std::string, std::vector<std::string>> dependents;

        for (const auto& [name, _] : m_RenderPasses)
        {
            inDegree[name] = 0;
            dependents[name] = {};
        }

        for (const auto& [name, deps] : m_RenderPassDependencies)
        {
            for (const auto& dep : deps)
            {
                inDegree[name]++;
                dependents[dep].push_back(name);
            }
        }

        std::queue<std::string> q;

        for (const auto& name : m_PassInsertionOrder)
        {
            if (inDegree[name] == 0)
                q.push(name);
        }

        std::vector<std::string> topo;

        while (!q.empty())
        {
            auto current = q.front();
            q.pop();

            topo.push_back(current);

            for (const auto& dep : dependents[current])
            {
                if (--inDegree[dep] == 0)
                    q.push(dep);
            }
        }

        // Cycle check
        if (topo.size() != m_RenderPasses.size())
        {
            DEBUG_ERROR("RenderPass cycle detected!");
            return;
        }

        // Categorize
        std::vector<std::string> cat1, cat2, cat3, cat4;

        for (const auto& name : topo)
        {
            auto it = m_RenderPassDependencies.find(name);
            bool hasDeps = (it != m_RenderPassDependencies.end() && !it->second.empty());
            bool hasDependents = !dependents[name].empty();

            if (!hasDeps && !hasDependents)
                cat1.push_back(name);
            else if (!hasDeps && hasDependents)
                cat2.push_back(name);
            else if (hasDeps && hasDependents)
                cat3.push_back(name);
            else
                cat4.push_back(name);
        }

        // Merge everything
        m_ExecutionOrder.clear();
        m_ExecutionOrder.insert(m_ExecutionOrder.end(), cat1.begin(), cat1.end());
        m_ExecutionOrder.insert(m_ExecutionOrder.end(), cat2.begin(), cat2.end());
        m_ExecutionOrder.insert(m_ExecutionOrder.end(), cat3.begin(), cat3.end());
        m_ExecutionOrder.insert(m_ExecutionOrder.end(), cat4.begin(), cat4.end());
    }

    void Renderer::Render()
    {
        BeginFrame();
        DrawFrame();
        EndFrame();
    }

    void Renderer::Shutdown()
    {
        m_ViewportBuffer->Destroy();
        m_LightManager->Clear();
    }

    uint64_t Renderer::GenerateSortKey(const DrawCommand& cmd, const uint32_t submeshID)
    {
        uint64_t key = 0;

        uint64_t pipelineID = (uint64_t)cmd.material->GetPipeline().get();
        uint64_t materialID = cmd.material->GetAssetID().GetAsInt();
        uint64_t meshID     = cmd.mesh->GetAssetID().GetAsInt();

        pipelineID = (pipelineID >> 4) & 0xFFFF;
        materialID = (materialID >> 4) & 0xFFFF;
        meshID     = (meshID >> 4) & 0xFFFF;

        key |= (pipelineID << 48);
        key |= (materialID << 32);
        key |= (meshID << 16);
        key |= (cmd.modelID + submeshID & 0xFFFF);

        return key;
    }

    std::shared_ptr<Pipeline> Renderer::GetOrAdd(const PipelineSpecifications& specs)
    {
        auto [it, inserted] = m_Pipelines.try_emplace(specs, nullptr);

        if (inserted)
        {
            it->second = Pipeline::Create(specs);
        }

        return it->second;
    }

    void Renderer::ReorderDrawList()
    {
        
    }

    void Renderer::AddCommands(const std::vector<DrawCommand>& commands, const std::vector<std::string>& passes)
    {
        for(const auto& passName : passes){
            auto pass = m_RenderPasses.find(passName);

            if(pass != m_RenderPasses.end()){
                for(int submeshID = 0; submeshID < commands.size(); submeshID++)
                {
                    auto cmd = commands[submeshID];
                    
                    if(!cmd.fullscreenTri)
                    {
                        uint64_t cmdID =
                            ((uint64_t)(cmd.mesh->GetAssetID().GetAsInt() & 0xFFFF) << 48) |
                            ((uint64_t)(cmd.modelID & 0xFFFFFFFF) << 16) |
                            ((uint64_t)(submeshID & 0xFFFF));
                        cmd.commandID = cmdID;
                        cmd.sortKey = GenerateSortKey(cmd, submeshID);
                    }

                    auto it = pass->second->drawCommandsLookup.find(cmd.commandID);

                    if (it != pass->second->drawCommandsLookup.end())
                    {
                        pass->second->drawList[it->second] = cmd;
                    }
                    else
                    {
                        pass->second->drawCommandsLookup[cmd.commandID] = pass->second->drawList.size();
                        pass->second->drawList.push_back(cmd);
                    }
                }

                pass->second->drawListDirty = true;
            }
            else{
                DEBUG_WARNING("Tried adding a draw command to a non-registered draw pass. Always register the pass before the draw commands ! passName : ", passName);
            }
        }
    }

    void Renderer::RemoveCommands(const std::vector<uint32_t> commandsID, const std::vector<std::string>& passes)
    {
        for(auto passName : passes){

            auto pass = m_RenderPasses.find(passName);

            if(pass != m_RenderPasses.end()){
                for(uint32_t id : commandsID)
                {
                    auto it = pass->second->drawCommandsLookup.find(id);
                    if (it == pass->second->drawCommandsLookup.end())
                        return;

                    size_t index = it->second;
                    size_t lastIndex = pass->second->drawList.size() - 1;

                    if (index != lastIndex)
                    {
                        pass->second->drawList[index] = pass->second->drawList[lastIndex];
                        pass->second->drawCommandsLookup[pass->second->drawList[index].commandID] = index;
                    }

                    pass->second->drawList.pop_back();
                    pass->second->drawCommandsLookup.erase(it);
                }
            }
            else{
                DEBUG_WARNING("Tried removing a draw command to a non-registered draw pass. Always register the pass before removing the draw commands !");
            }

        }
    }

    void Renderer::ForceDrawlistUpdate(const std::vector<std::string>& passes)
    {
        for(auto passName : passes){
            auto it = m_RenderPasses.find(passName);
            if(it != m_RenderPasses.end()){
                it->second->drawListDirty = true;
            }
        }
    }

    void Renderer::AddRenderPass(const std::shared_ptr<RenderPass> pass, const std::string& name, const std::vector<std::string>& dependencies)
    {
        if (!pass)
        {
            DEBUG_ERROR("Trying to add null RenderPass: " + name);
            return;
        }

        if(m_RenderPasses.find(name) == m_RenderPasses.end()){
            m_PassInsertionOrder.push_back(name);
        }

        // Replace or insert
        m_RenderPasses[name] = pass;

        // Set dependencies
        m_RenderPassDependencies[name] = dependencies;

        BuildExecutionOrder();
    }

    void Renderer::RemoveRenderPass(const std::string &name)
    {
        auto it = m_RenderPasses.find(name);

        if(it != m_RenderPasses.end()){
            m_RenderPasses.erase(name);
        }
        else{
            DEBUG_ERROR("Tried removing non-registered render pass");
        }
    }

    void Renderer::RescaleFramebuffers(int newWidth, int newHeight)
    {
        for(auto renderPass : m_RenderPasses){
            if(renderPass.second->allowResize){
                renderPass.second->target->Resize(newWidth, newHeight);
            }
        }

        Core::GetEngine().GetCameraManager()->UpdateSize(newWidth, newHeight);
    }

    void Renderer::ToggleMultisampling(const bool on)
    {
        m_MultisamplingEnabled = on;
        m_RendererAPI->ToggleMultisampling(on);
    }

    std::string Renderer::GetDeviceVendor()
    {
        return m_RendererAPI->GetDeviceVendor();
    }

    std::string Renderer::GetRendererName()
    {
        return m_RendererAPI->GetRendererName();
    }

    std::string Renderer::GetDriverVersion()
    {
        return m_RendererAPI->GetDriverVersion();
    }

    void Renderer::BeginFrame()
    {
        Core::GetEngine().GetCameraManager()->Tick();
        ReorderDrawList();
        m_ShadowManager->UpdatePassUniforms();
    }

    void Renderer::DrawFrame()
    {
        for (const auto& passName : m_ExecutionOrder)
        {
            auto pass = m_RenderPasses[passName];
            BeginRenderPass(pass);
            ExecuteRenderPass();
            EndRenderPass();
        }

    }

    void Renderer::EndFrame()
    {
        m_ViewportBuffer->ResolveMultisampled();
    }

    void Renderer::BeginRenderPass(const std::shared_ptr<RenderPass>& pass)
    {
        m_CurrentPass = pass;
        pass->target->Bind();
        m_RendererAPI->SetViewport(0, 0, pass->target->GetWidth(), pass->target->GetHeight());
        ClearBit clearMask = ClearBit::None;
        if(pass->clearColor)
            clearMask |= ClearBit::Color;
        if(pass->clearDepth)
            clearMask |= ClearBit::Depth;
        m_RendererAPI->Clear(clearMask);
    }

    void Renderer::ExecuteRenderPass()
    { 
        if (m_CurrentPass->drawList.empty())
            return;

        for(auto& drawCmd : m_CurrentPass->drawList){
            
            glm::vec3 center = (drawCmd.boundsMin + drawCmd.boundsMax) * 0.5f;
            glm::vec3 extents = (drawCmd.boundsMax - drawCmd.boundsMin) * 0.5f;

            glm::vec3 worldCenter = glm::vec3(drawCmd.modelMatrix * glm::vec4(center, 1.0));

            glm::mat3 m = glm::mat3(drawCmd.modelMatrix);

            glm::mat3 absM(
                glm::abs(m[0]),
                glm::abs(m[1]),
                glm::abs(m[2])
            );

            glm::vec3 worldExtents = absM * extents;

            glm::vec3 worldMin = worldCenter - worldExtents;
            glm::vec3 worldMax = worldCenter + worldExtents;

            auto camera = Core::GetEngine().GetCameraManager()->GetActiveCamera();

            if (!drawCmd.material || drawCmd.indexCount <= 0)
                continue;

            if (camera->frustumCulling && worldMin != worldMax && m_CurrentPass->allowCulling)
            {
                if (!camera->IsInFrustum(worldMin, worldMax))
                    continue;
            }

            m_RendererAPI->ExecuteDrawCommand(drawCmd, m_CurrentPass);
        }
    }

    void Renderer::EndRenderPass()
    {
    }
}