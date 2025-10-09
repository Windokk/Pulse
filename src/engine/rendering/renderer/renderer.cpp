#include "renderer.hpp"

#include <iostream>
#include <algorithm>

#include "engine/levels/level_manager.hpp"

#include "engine/ecs/objects/actors/actor.hpp"

#include "engine/core/engine.hpp"

#include "engine/rendering/camera/camera_manager.hpp"

#include "engine/core/resources/resources_manager.hpp"

namespace Epoch::Engine::Rendering{
    
    void Renderer::CreateRectGeometry()
    {

        float rectVertices[] = {
            // positions     // texCoords
            -1.0f,  1.0f,     0.0f, 1.0f, // top-left
            -1.0f, -1.0f,     0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,     1.0f, 0.0f, // bottom-right
        
            -1.0f,  1.0f,     0.0f, 1.0f, // top-left
             1.0f, -1.0f,     1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,     1.0f, 1.0f  // top-right
        };
        
        GetGL().GenVertexArrays(1, &rectVAO);
        GetGL().GenBuffers(1, &rectVBO);
        
        GetGL().BindVertexArray(rectVAO);
        
        GetGL().BindBuffer(GL_ARRAY_BUFFER, rectVBO);
        GetGL().BufferData(GL_ARRAY_BUFFER, sizeof(rectVertices), rectVertices, GL_STATIC_DRAW);
        
        // position (vec2)
        GetGL().VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        GetGL().EnableVertexAttribArray(0);
        // texCoords (vec2)
        GetGL().VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        GetGL().EnableVertexAttribArray(1);
        
        GetGL().BindVertexArray(0);

    }

    void Renderer::Init(RendererSettings settings)
    {
        settings.windowWidth = Core::GetEngine().GetWindow()->GetFramebufferWidth();
        settings.windowHeight = Core::GetEngine().GetWindow()->GetFramebufferHeight();

        GetGL().Viewport(0, 0, settings.windowWidth, settings.windowHeight);

        this->settings = settings;

        //FRAMBUFFERS
        CreateRectGeometry();

        //LIGHTS
        lightMan = new LightManager();
        lightMan->Update(-1);
        
        //MULTISAMPLING
        GetGL().Enable(GL_MULTISAMPLE);

        //SHADOWS
        shadowMan = new ShadowManager();
        shadowMan->Init(4096);
    }

    void Renderer::InitFramebuffers()
    {
        blendShader = Core::Resources::ResourcesManager::GetInstance().GetShader("shaders\\fb\\blend");
        framebufferShader = Core::Resources::ResourcesManager::GetInstance().GetShader("shaders\\fb\\framebuffer");

        if(framebufferShader == nullptr || blendShader == nullptr){
            DEBUG_ERROR("Viewport buffer cannot be created if the framebuffer shader or the blend shader are null");
        }
        else{
            viewportBuffer = new FrameBuffer(settings.windowWidth, settings.windowHeight, framebufferShader, true);
        }

        //DEBUG_SHAPES
        Renderer::unlitShader = Core::Resources::ResourcesManager::GetInstance().GetShader("shaders\\mesh\\unlit");
    }

    void Renderer::Shutdown()
    {
        for(RenderPass& renderPass : renderPasses){
            renderPass.target->Shutdown();
        }
        viewportBuffer->Shutdown();
    }

    void Renderer::Render()
    {
        if(CameraManager::GetInstance().GetActiveCamera() != nullptr && Levels::LevelManager::GetInstance().GetLoadedLevelCount() > 0){
            BeginFrame();

            ExecuteRenderPasses();
        }
    }

    void Renderer::SubmitCommand(DrawCommand cmd, bool replace)
    {
        if(replace){
            for(int i = 0; i < drawList.size(); i++){ 
                if(drawList[i].id == cmd.id){
                    drawList[i] = cmd;
                }
            }
        }
        else{
            drawList.push_back(cmd);
        }

        ReorderDrawList();
    }

    void Renderer::SubmitCommands(std::vector<DrawCommand> cmds, bool replace)
    {
        if (replace) {
            for (const auto& cmd : cmds) {
                bool found = false;
                for (auto& existingCmd : drawList) {
                    if (existingCmd.id == cmd.id) {
                        existingCmd = cmd;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    drawList.push_back(cmd);
                }
            }
        } else {
            drawList.insert(drawList.end(), cmds.begin(), cmds.end());
        }

        ReorderDrawList();
    }

    void Renderer::RemoveCommand(DrawCommand cmd)
    {
        for(int i = 0; i < drawList.size(); i++){ 
            if(drawList[i].id == cmd.id){
                drawList.erase(drawList.begin()+i);
            }
        }

        ReorderDrawList();
    }

    void Renderer::RemoveCommands(std::vector<DrawCommand> cmds)
    {
        std::unordered_set<int> idsToRemove;
        for (const auto& cmd : cmds) {
            idsToRemove.insert(cmd.id);
        }

        drawList.erase(
            std::remove_if(drawList.begin(), drawList.end(),
                        [&](const DrawCommand& dc) {
                            return idsToRemove.count(dc.id) > 0;
                        }),
            drawList.end()
        );

        ReorderDrawList();
    }

    void Renderer::ReorderDrawList()
    {
        drawList.erase(
            std::remove_if(drawList.begin(), drawList.end(),
                [](const DrawCommand& cmd) {
                    return cmd.tr == nullptr;
                }),
            drawList.end()
        );

        std::stable_partition(drawList.begin(), drawList.end(),
        [](const DrawCommand& cmd) {
            return cmd.mat->renderMode != RenderMode::TRANSLUCENT;
        });
    }

    void Renderer::BeginFrame()
    {

        CameraManager::GetInstance().Tick();

        if(settings.enableShadows){
            auto& levelManager = Levels::LevelManager::GetInstance();

            size_t totalMeshCount = 0;
            for (int i = 0, count = levelManager.GetLoadedLevelCount(); i < count; ++i)
                totalMeshCount += levelManager.GetLevelAt(i)->meshes.size();

            std::vector<std::pair<glm::mat4, Epoch::Engine::Rendering::Mesh*>> allMeshes;
            allMeshes.reserve(totalMeshCount);

            for (int i = 0, count = levelManager.GetLoadedLevelCount(); i < count; ++i) {
                const auto* level = levelManager.GetLevelAt(i);
                for (const auto& [id, meshData] : level->meshes) {
                    allMeshes.push_back(meshData);
                }
            }
 
            shadowMan->RenderShadowMaps(allMeshes, CameraManager::GetInstance().GetActiveCamera());

            Renderer::settings.windowWidth = Core::GetEngine().GetWindow()->GetFramebufferWidth();
            Renderer::settings.windowHeight = Core::GetEngine().GetWindow()->GetFramebufferHeight();
            GetGL().Viewport(0, 0, Renderer::settings.windowWidth, Renderer::settings.windowHeight);
        }

        GetGL().ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        GetGL().Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        GetGL().Enable(GL_CULL_FACE);
        GetGL().CullFace(GL_BACK);
        GetGL().FrontFace(GL_CCW);
    }
  
    void Renderer::DrawScene()
    {
        GetGL().Enable(GL_DEPTH_TEST);
        GetGL().ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        GetGL().Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --- Main Draw Calls ---
        for (auto& cmd : drawList) {
            if (!cmd.mat || cmd.indexCount <= 0 || (CameraManager::GetInstance().GetActiveCamera()->frustumCulling && !CameraManager::GetInstance().GetActiveCamera()->IsInFrustum(cmd.boundsMin, cmd.boundsMax)))
                continue;

            if(settings.enableShadows && cmd.mat->recievesShadows)
                shadowMan->BindShadowMaps(cmd.mat);

            switch (cmd.mat->renderMode)
            {
                case TRANSLUCENT:
                    GetGL().Enable(GL_BLEND);
                    GetGL().BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    cmd.mat->SetParameter("masked", false);
                    break;

                case MASKED:
                    cmd.mat->SetParameter("masked", true);
                    GetGL().Disable(GL_BLEND);
                    break;

                case OPAQUE:
                    cmd.mat->SetParameter("masked", false);
                    GetGL().Disable(GL_BLEND);
                    break;
                    
                default:
                    break;
            }

            cmd.mat->SetParameter("projection", CameraManager::GetInstance().GetActiveCamera()->GetProjection());
            cmd.mat->SetParameter("view", CameraManager::GetInstance().GetActiveCamera()->GetView());
            cmd.mat->SetParameter("model", cmd.tr->GetTransformMatrix());
            cmd.mat->SetParameter("lightNB", lightMan->GetLightsCount());
            cmd.mat->SetParameter("camPos", CameraManager::GetInstance().GetActiveCamera()->parent->transform->GetPosition());
            cmd.mat->Use();
            GetGL().BindVertexArray(cmd.VAO);
            GetGL().PolygonMode(GL_FRONT_AND_BACK, cmd.fillMode);
            GetGL().DrawElements(GL_TRIANGLES, cmd.indexCount, GL_UNSIGNED_INT, (void*)(cmd.indexOffset * sizeof(uint32_t)));
            cmd.mat->StopUsing();
        }

        if(!Renderer::settings.showDebugShapes){
            GetGL().BindVertexArray(0);
            GetGL().UseProgram(0);
            GetGL().Disable(GL_DEPTH_TEST);
            return;
        }

        // --- Debug Physics Shapes ---
        unlitShader->Activate();
        unlitShader->setMat4("projectionView", CameraManager::GetInstance().GetActiveCamera()->GetMatrix());

        for (auto& physicBody : Levels::LevelManager::GetInstance().GetLevelAt(0)->physicsBodies) {

            glm::vec3 pos = physicBody->parent->transform->GetPosition();
            glm::quat rot = physicBody->parent->transform->GetRotation();

            glm::mat4 model = glm::translate(glm::mat4(1.0f), pos) * glm::toMat4(rot);

            unlitShader->setMat4("model", model);

            GetGL().BindVertexArray(physicBody->GetDebugShape()->GetVAO());
            GetGL().DrawElements(GL_LINES, physicBody->GetDebugShape()->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
        }

        GetGL().BindVertexArray(0);
        GetGL().UseProgram(0);
        GetGL().Disable(GL_DEPTH_TEST);
    }

    void Renderer::RescaleFramebuffers(int newWidth, int newHeight)
    {
        for (auto& pass : renderPasses)
            if(pass.target != nullptr){
                pass.target->RescaleFrameBuffer(newWidth, newHeight);
            }

        viewportBuffer->RescaleFrameBuffer(newWidth, newHeight);

        GetGL().Viewport(0, 0, newWidth, newHeight);

        settings.windowWidth = newWidth;
        settings.windowHeight = newHeight;

        CameraManager::GetInstance().UpdateSize(newWidth, newHeight);
    }

    void Renderer::AddRenderPass(RenderStage stage, std::function<void()> callback, std::shared_ptr<FrameBuffer> fb, bool appendToViewport, BlendMode blendMode)
    {
        renderPasses.push_back(RenderPass{stage, std::move(callback), fb, appendToViewport, blendMode});
    
        std::sort(renderPasses.begin(), renderPasses.end(),
            [](const RenderPass& a, const RenderPass& b) {
                return static_cast<int>(a.stage) < static_cast<int>(b.stage);
            });
    }

    void Renderer::ExecuteRenderPasses(){
        for (const auto& pass : renderPasses) {
            switch (pass.stage) {
                case RenderStage::UI:{
                    // No FBO
                    pass.callback();
                    break;
                }
                case RenderStage::PostProcess:{
                    if(!pass.target)
                        DEBUG_ERROR("Couldn't render pass without framebuffer");

                    if(!Renderer::settings.enablePostProcessing)
                        break;

                    pass.target->Bind();
                    viewportBuffer->Draw(rectVAO);
                    pass.target->Unbind();
                    
                    if(pass.appendToViewport){
                        viewportBuffer->Bind();
                    }
                        pass.callback();
                        pass.target->Draw(rectVAO);

                    if(pass.appendToViewport){
                        viewportBuffer->Unbind();
                        viewportBuffer->Draw(rectVAO);
                    }

                    break;
                }
                case RenderStage::Scene:
                default:{
                    if(!pass.target)
                        DEBUG_ERROR("Couldn't render pass without framebuffer");
                    pass.target->Bind();
                    pass.callback();
                    pass.target->Unbind();

                    if (pass.target->isMultisampled)
                        pass.target->Resolve();

                    pass.target->Draw(rectVAO);

                    if(pass.appendToViewport){
                        viewportBuffer->Bind();

                        GetGL().ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                        GetGL().Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                        
                        blendShader->Activate();

                        blendShader->setInt("blendMode", (int)pass.blendMode);
                        blendShader->setInt("texA", 0);
                        blendShader->setInt("texB", 1);

                        GetGL().ActiveTexture(GL_TEXTURE0);
                        GetGL().BindTexture(GL_TEXTURE_2D, viewportBuffer->GetFrameTexture());

                        GetGL().ActiveTexture(GL_TEXTURE1);
                        if(pass.target->isMultisampled)
                            GetGL().BindTexture(GL_TEXTURE_2D, pass.target->GetFrameTexture());
                        else
                            GetGL().BindTexture(GL_TEXTURE_2D, pass.target->GetFrameTexture());
        
                        GetGL().BindVertexArray(rectVAO);
                        GetGL().Disable(GL_DEPTH_TEST);
                        
                        GetGL().DrawArrays(GL_TRIANGLES, 0, 6);
                        
                        GetGL().BindVertexArray(0);

                        blendShader->Deactivate();

                        GetGL().ActiveTexture(GL_TEXTURE0);
                        GetGL().BindTexture(GL_TEXTURE_2D, 0);
                        GetGL().ActiveTexture(GL_TEXTURE1);
                        GetGL().BindTexture(GL_TEXTURE_2D, 0);

                        viewportBuffer->Unbind();

                        viewportBuffer->Draw(rectVAO);

                    }

                    break;
                }
            }
        }

    }

}