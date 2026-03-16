#include "renderer.hpp"

#include "engine/core/engine.hpp"

#include "engine/rendering/material/material.hpp"
#include "engine/rendering/pipeline/pipeline.hpp"
#include "engine/rendering/shader/shader.hpp"
#include "engine/rendering/mesh/mesh.hpp"
#include "engine/rendering/lighting/shadow_manager.hpp"
#include "engine/rendering/camera/camera_manager.hpp"

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
        VertexLayout basicVertexLayout{
            {"aPos",       ShaderDataType::Vec3, 0},
            {"aTexCoord", ShaderDataType::Vec2, 1}
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
        m_UnitCube->Create(cubeVertices, cubeIndices, basicVertexLayout);
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
        m_UnitQuad->Create(quadVertices, quadIndices, basicVertexLayout);

        // Init lighting
        m_LightManager = std::make_shared<LightManager>();
        m_ShadowManager = std::make_shared<ShadowManager>();
        m_ShadowManager->Init(512, 1024, 2048);

        
        //Init built-in passes
        /// Shadow passes (sent from the shadow manager)

        /// Forward pass
        std::shared_ptr<RenderPass> forwardPass = std::make_shared<RenderPass>();
        forwardPass->target = m_ViewportBuffer;
        forwardPass->clearColor = true;
        forwardPass->clearDepth = true;
        forwardPass->overridePipeline = false;
        AddRenderPass(forwardPass, "ForwardPass");

        /// @note for now we're forced to submit the forward after the shadow passes "manually", in the future, we'll create a render graph with dependencies
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

    void Renderer::ReorderDrawList()
    {
        for(auto [name, pass] : m_RenderPasses){
            if(!pass->drawListDirty)
                continue;
            std::sort(pass->drawList.begin(), pass->drawList.end(),
                [](const DrawCommand& a, const DrawCommand& b)
                {
                    return a.sortKey < b.sortKey;
                });

            pass->drawListDirty = false;
        }

    }

    void Renderer::AddCommands(const std::vector<DrawCommand>& commands, const std::vector<std::string>& passes)
    {
        for(const auto& passName : passes){
            auto pass = m_RenderPasses.find(passName);

            if(pass != m_RenderPasses.end()){
                for(int i = 0; i < commands.size(); i++)
                {
                    auto cmd = commands[i];
                    
                    if(!cmd.fullscreenTri)
                    {
                        uint64_t cmdID = ((uint64_t)cmd.modelID << 16) | (uint64_t)i;
                        cmd.commandID = cmdID;
                        cmd.sortKey = GenerateSortKey(cmd, i);
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

    void Renderer::AddRenderPass(const std::shared_ptr<RenderPass> pass, const std::string &name)
    {
        auto it = m_RenderPasses.find(name);

        if(it != m_RenderPasses.end()){
            m_RenderPasses[it->first] = pass;
        }
        else{
            m_RenderPasses.emplace(name, pass);
        }
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
            renderPass.second->target->Resize(newWidth, newHeight);
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
        ReorderDrawList();
        m_ShadowManager->UpdatePassUniforms();
    }

    void Renderer::DrawFrame()
    {
        for(auto [id, pass] : m_RenderPasses){
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
        for(auto& drawCmd : m_CurrentPass->drawList){

            glm::mat4 M = drawCmd.modelMatrix;

            glm::vec3 corners[8] = {
                M * glm::vec4(drawCmd.boundsMin.x, drawCmd.boundsMin.y, drawCmd.boundsMin.z, 1.0),
                M * glm::vec4(drawCmd.boundsMin.x, drawCmd.boundsMin.y, drawCmd.boundsMax.z, 1.0),
                M * glm::vec4(drawCmd.boundsMin.x, drawCmd.boundsMax.y, drawCmd.boundsMin.z, 1.0),
                M * glm::vec4(drawCmd.boundsMin.x, drawCmd.boundsMax.y, drawCmd.boundsMax.z, 1.0),
                M * glm::vec4(drawCmd.boundsMax.x, drawCmd.boundsMin.y, drawCmd.boundsMin.z, 1.0),
                M * glm::vec4(drawCmd.boundsMax.x, drawCmd.boundsMin.y, drawCmd.boundsMax.z, 1.0),
                M * glm::vec4(drawCmd.boundsMax.x, drawCmd.boundsMax.y, drawCmd.boundsMin.z, 1.0),
                M * glm::vec4(drawCmd.boundsMax.x, drawCmd.boundsMax.y, drawCmd.boundsMax.z, 1.0)
            };

            glm::vec3 worldMin( std::numeric_limits<float>::max() );
            glm::vec3 worldMax( std::numeric_limits<float>::lowest() );

            for (int i = 0; i < 8; i++) {
                worldMin = glm::min(worldMin, corners[i]);
                worldMax = glm::max(worldMax, corners[i]);
            }

            if (!drawCmd.material || drawCmd.indexCount <= 0 || (Core::GetEngine().GetCameraManager()->GetActiveCamera()->frustumCulling && !Core::GetEngine().GetCameraManager()->GetActiveCamera()->IsInFrustum(worldMin, worldMax)))
                continue;

            m_RendererAPI->ExecuteDrawCommand(drawCmd, m_CurrentPass);
        }
    }

    void Renderer::EndRenderPass()
    {
    }
}