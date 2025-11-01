#include "renderer.hpp"

#include <iostream>
#include <algorithm>

#include "engine/levels/level_manager.hpp"

#include "engine/ecs/objects/actors/actor.hpp"

#include "engine/core/engine.hpp"

#include "engine/rendering/camera/camera_manager.hpp"

#include "engine/core/resources/resources_manager.hpp"

namespace Pulse::Engine::Rendering{
    
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
        
        Core::GetEngine().GetGL()->GenVertexArrays(1, &rectVAO);
        Core::GetEngine().GetGL()->GenBuffers(1, &rectVBO);
        
        Core::GetEngine().GetGL()->BindVertexArray(rectVAO);
        
        Core::GetEngine().GetGL()->BindBuffer(GL_ARRAY_BUFFER, rectVBO);
        Core::GetEngine().GetGL()->BufferData(GL_ARRAY_BUFFER, sizeof(rectVertices), rectVertices, GL_STATIC_DRAW);
        
        // position (vec2)
        Core::GetEngine().GetGL()->VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        Core::GetEngine().GetGL()->EnableVertexAttribArray(0);
        // texCoords (vec2)
        Core::GetEngine().GetGL()->VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        Core::GetEngine().GetGL()->EnableVertexAttribArray(1);
        
        Core::GetEngine().GetGL()->BindVertexArray(0);

    }

    void Renderer::Init(RendererSettings settings)
    {
        settings.windowWidth = Core::GetEngine().GetWindow()->GetFramebufferWidth();
        settings.windowHeight = Core::GetEngine().GetWindow()->GetFramebufferHeight();

        Core::GetEngine().GetGL()->Viewport(0, 0, settings.windowWidth, settings.windowHeight);

        this->settings = settings;

        //FRAMBUFFERS
        CreateRectGeometry();

        //LIGHTS
        lightMan = new LightManager();
        lightMan->Update(-1);
        
        //MULTISAMPLING
        Core::GetEngine().GetGL()->Enable(GL_MULTISAMPLE);

        //SHADOWS
        shadowMan = new ShadowManager();
        shadowMan->Init(512, 1024, 2048);
    }

    void Renderer::InitFramebuffers()
    {
        blendShader = Core::GetEngine().GetResourcesManager()->GetShader("shaders\\fb\\blend");
        framebufferShader = Core::GetEngine().GetResourcesManager()->GetShader("shaders\\fb\\framebuffer");

        if(framebufferShader == nullptr || blendShader == nullptr){
            DEBUG_ERROR("Viewport buffer cannot be created if the framebuffer shader or the blend shader are null");
        }
        else{
            viewportBuffer = new FrameBuffer(settings.windowWidth, settings.windowHeight, framebufferShader, true);
        }

        //DEBUG_SHAPES
        Renderer::unlitShader = Core::GetEngine().GetResourcesManager()->GetShader("shaders\\mesh\\unlit");

        initialized = true;
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
        if(Core::GetEngine().GetCameraManager()->GetActiveCamera() != nullptr && Core::GetEngine().GetLevelManager()->GetLoadedLevelCount() > 0){
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
            return cmd.mat != nullptr && cmd.mat->renderMode != RenderMode::TRANSLUCENT;
        });
    }

    void Renderer::BeginFrame()
    {

        Core::GetEngine().GetCameraManager()->Tick();

        if(settings.enableShadows){
            auto levelManager = Core::GetEngine().GetLevelManager();

            size_t totalMeshCount = 0;
            for (int i = 0, count = levelManager->GetLoadedLevelCount(); i < count; ++i)
                totalMeshCount += levelManager->GetLevelAt(i)->meshes.size();

            std::vector<std::pair<glm::mat4, Pulse::Engine::Rendering::Mesh*>> allMeshes;
            allMeshes.reserve(totalMeshCount);

            for (int i = 0, count = levelManager->GetLoadedLevelCount(); i < count; ++i) {
                const auto* level = levelManager->GetLevelAt(i);
                for (const auto& [id, meshData] : level->meshes) {
                    allMeshes.push_back(meshData);
                }
            }
 
            shadowMan->RenderShadowMaps(allMeshes, Core::GetEngine().GetCameraManager()->GetActiveCamera());

            Renderer::settings.windowWidth = Core::GetEngine().GetWindow()->GetFramebufferWidth();
            Renderer::settings.windowHeight = Core::GetEngine().GetWindow()->GetFramebufferHeight();
            Core::GetEngine().GetGL()->Viewport(0, 0, Renderer::settings.windowWidth, Renderer::settings.windowHeight);
        }

        Core::GetEngine().GetGL()->ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        Core::GetEngine().GetGL()->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Core::GetEngine().GetGL()->Enable(GL_CULL_FACE);
        Core::GetEngine().GetGL()->CullFace(GL_BACK);
        Core::GetEngine().GetGL()->FrontFace(GL_CCW);
    }
  
    void Renderer::DrawScene()
    {
        Core::GetEngine().GetGL()->Enable(GL_DEPTH_TEST);
        Core::GetEngine().GetGL()->ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        Core::GetEngine().GetGL()->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --- Main Draw Calls ---
        for (auto& cmd : drawList) {
            if (!cmd.mat || cmd.indexCount <= 0 || (Core::GetEngine().GetCameraManager()->GetActiveCamera()->frustumCulling && !Core::GetEngine().GetCameraManager()->GetActiveCamera()->IsInFrustum(cmd.boundsMin, cmd.boundsMax)))
                continue;

            if(settings.enableShadows && cmd.mat->recievesShadows)
                shadowMan->BindShadowMaps(cmd.mat);

            switch (cmd.mat->renderMode)
            {
                case TRANSLUCENT:
                    Core::GetEngine().GetGL()->Enable(GL_BLEND);
                    Core::GetEngine().GetGL()->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    cmd.mat->SetParameter("masked", false);
                    break;

                case MASKED:
                    cmd.mat->SetParameter("masked", true);
                    Core::GetEngine().GetGL()->Disable(GL_BLEND);
                    break;

                case OPAQUE:
                    cmd.mat->SetParameter("masked", false);
                    Core::GetEngine().GetGL()->Disable(GL_BLEND);
                    break;
                    
                default:
                    break;
            }

            cmd.mat->SetParameter("projection", Core::GetEngine().GetCameraManager()->GetActiveCamera()->GetProjection());
            cmd.mat->SetParameter("view", Core::GetEngine().GetCameraManager()->GetActiveCamera()->GetView());
            cmd.mat->SetParameter("model", cmd.tr->GetTransformMatrix());
            cmd.mat->SetParameter("lightNB", lightMan->GetLightsCount());
            cmd.mat->SetParameter("camPos", Core::GetEngine().GetCameraManager()->GetActiveCamera()->parent->transform->GetPosition());
            cmd.mat->Use();
            Core::GetEngine().GetGL()->BindVertexArray(cmd.VAO);
            Core::GetEngine().GetGL()->PolygonMode(GL_FRONT_AND_BACK, cmd.fillMode);
            Core::GetEngine().GetGL()->DrawElements(GL_TRIANGLES, cmd.indexCount, GL_UNSIGNED_INT, (void*)(cmd.indexOffset * sizeof(uint32_t)));
            cmd.mat->StopUsing();
        }

        if(!Renderer::settings.showDebugShapes){
            Core::GetEngine().GetGL()->BindVertexArray(0);
            Core::GetEngine().GetGL()->UseProgram(0);
            Core::GetEngine().GetGL()->Disable(GL_DEPTH_TEST);
            return;
        }

        // --- Debug Physics Shapes ---
        unlitShader->Activate();
        unlitShader->setMat4("projectionView", Core::GetEngine().GetCameraManager()->GetActiveCamera()->GetMatrix());

        for (auto& physicBody : Core::GetEngine().GetLevelManager()->GetLevelAt(0)->physicsBodies) {

            glm::vec3 pos = physicBody->parent->transform->GetPosition();
            glm::quat rot = physicBody->parent->transform->GetRotation();

            glm::mat4 model = glm::translate(glm::mat4(1.0f), pos) * glm::toMat4(rot);

            unlitShader->setMat4("model", model);

            Core::GetEngine().GetGL()->BindVertexArray(physicBody->GetDebugShape()->GetVAO());
            Core::GetEngine().GetGL()->DrawElements(GL_LINES, physicBody->GetDebugShape()->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
        }

        Core::GetEngine().GetGL()->BindVertexArray(0);
        Core::GetEngine().GetGL()->UseProgram(0);
        Core::GetEngine().GetGL()->Disable(GL_DEPTH_TEST);
    }

    void Renderer::RescaleFramebuffers(int newWidth, int newHeight)
    {
        for (auto& pass : renderPasses){
            if(pass.target != nullptr){
                pass.target->RescaleFrameBuffer(newWidth, newHeight);
            }
        }
        
        if(!viewportBuffer){
            DEBUG_FATAL("Called \"RescaleFramebuffers\" with null viewportBuffer");
            return;
        }

        viewportBuffer->RescaleFrameBuffer(newWidth, newHeight);

        Core::GetEngine().GetGL()->Viewport(0, 0, newWidth, newHeight);

        settings.windowWidth = newWidth;
        settings.windowHeight = newHeight;

        Core::GetEngine().GetCameraManager()->UpdateSize(newWidth, newHeight);
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

                        Core::GetEngine().GetGL()->ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                        Core::GetEngine().GetGL()->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                        
                        blendShader->Activate();

                        blendShader->setInt("blendMode", (int)pass.blendMode);
                        blendShader->setInt("texA", 0);
                        blendShader->setInt("texB", 1);

                        Core::GetEngine().GetGL()->ActiveTexture(GL_TEXTURE0);
                        Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_2D, viewportBuffer->GetFrameTexture());

                        Core::GetEngine().GetGL()->ActiveTexture(GL_TEXTURE1);
                        if(pass.target->isMultisampled)
                            Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_2D, pass.target->GetFrameTexture());
                        else
                            Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_2D, pass.target->GetFrameTexture());
        
                        Core::GetEngine().GetGL()->BindVertexArray(rectVAO);
                        Core::GetEngine().GetGL()->Disable(GL_DEPTH_TEST);
                        
                        Core::GetEngine().GetGL()->DrawArrays(GL_TRIANGLES, 0, 6);
                        
                        Core::GetEngine().GetGL()->BindVertexArray(0);

                        blendShader->Deactivate();

                        Core::GetEngine().GetGL()->ActiveTexture(GL_TEXTURE0);
                        Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_2D, 0);
                        Core::GetEngine().GetGL()->ActiveTexture(GL_TEXTURE1);
                        Core::GetEngine().GetGL()->BindTexture(GL_TEXTURE_2D, 0);

                        viewportBuffer->Unbind();

                        viewportBuffer->Draw(rectVAO);

                    }

                    break;
                }
            }
        }

    }

}