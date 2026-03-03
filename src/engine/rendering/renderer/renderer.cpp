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

        //RENDERING LIMITS
        Core::GetEngine().GetGL()->GetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTextures);

        //DEFAULTS
        defaultShader = Core::GetEngine().GetResourcesManager()->GetShader("shaders/mesh/unlit");

        GLenum filter[2] = {GL_LINEAR,GL_LINEAR};
        GLenum wrapC[3] = {GL_REPEAT,GL_REPEAT,GL_REPEAT};
        GLenum wrapT[2] = {GL_REPEAT,GL_REPEAT};
        unsigned char* cubemapData[6] = {nullptr,nullptr,nullptr,nullptr,nullptr,nullptr};

        defaultCubemap = std::make_shared<Cubemap>(1, 1, filter, wrapC, GL_RGB, GL_RGB, cubemapData, false);

        defaultTexture = std::make_shared<Texture>(1, 1, 3, filter, wrapT, GL_RGB, GL_RGB, nullptr, false);
    }

    void Renderer::InitFramebuffers()
    {
        blendShader = Core::GetEngine().GetResourcesManager()->GetShader("shaders/fb/blend");
        framebufferShader = Core::GetEngine().GetResourcesManager()->GetShader("shaders/fb/framebuffer");

        if(framebufferShader == nullptr || blendShader == nullptr){
            DEBUG_FATAL("Viewport buffer cannot be created if the framebuffer shader or the blend shader are null");
        }
        else{
            viewportBuffer = std::make_shared<FrameBuffer>(settings.windowWidth, settings.windowHeight, framebufferShader, true);
            tempBuffer = std::make_shared<FrameBuffer>(settings.windowWidth, settings.windowHeight, framebufferShader, false);
        }

        initialized = true;
    }

    void Renderer::Shutdown()
    {
        for(RenderPass& renderPass : renderPasses){
            renderPass.target->Shutdown();
        }
        viewportBuffer->Shutdown();
        lightMan->Clear();
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

    }
  
    void Renderer::DrawScene()
    {
        Core::GetEngine().GetGL()->Enable(GL_DEPTH_TEST);
        Core::GetEngine().GetGL()->ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        Core::GetEngine().GetGL()->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Core::GetEngine().GetGL()->Enable(GL_CULL_FACE);
        Core::GetEngine().GetGL()->CullFace(GL_BACK);
        Core::GetEngine().GetGL()->FrontFace(GL_CCW);

        // --- Main Draw Calls ---
        for (auto& cmd : drawList) {

            glm::mat4 M = cmd.tr->GetTransformMatrix();

            glm::vec3 corners[8] = {
                M * glm::vec4(cmd.boundsMin.x, cmd.boundsMin.y, cmd.boundsMin.z, 1.0),
                M * glm::vec4(cmd.boundsMin.x, cmd.boundsMin.y, cmd.boundsMax.z, 1.0),
                M * glm::vec4(cmd.boundsMin.x, cmd.boundsMax.y, cmd.boundsMin.z, 1.0),
                M * glm::vec4(cmd.boundsMin.x, cmd.boundsMax.y, cmd.boundsMax.z, 1.0),
                M * glm::vec4(cmd.boundsMax.x, cmd.boundsMin.y, cmd.boundsMin.z, 1.0),
                M * glm::vec4(cmd.boundsMax.x, cmd.boundsMin.y, cmd.boundsMax.z, 1.0),
                M * glm::vec4(cmd.boundsMax.x, cmd.boundsMax.y, cmd.boundsMin.z, 1.0),
                M * glm::vec4(cmd.boundsMax.x, cmd.boundsMax.y, cmd.boundsMax.z, 1.0)
            };

            glm::vec3 worldMin( std::numeric_limits<float>::max() );
            glm::vec3 worldMax( std::numeric_limits<float>::lowest() );

            for (int i = 0; i < 8; i++) {
                worldMin = glm::min(worldMin, corners[i]);
                worldMax = glm::max(worldMax, corners[i]);
            }

            if (!cmd.mat || cmd.indexCount <= 0 || (Core::GetEngine().GetCameraManager()->GetActiveCamera()->frustumCulling && !Core::GetEngine().GetCameraManager()->GetActiveCamera()->IsInFrustum(worldMin, worldMax)))
                continue;

            if(settings.enableShadows && cmd.mat->recievesShadows)
                shadowMan->BindShadowMaps(cmd.mat);

            switch (cmd.mat->renderMode)
            {
                case TRANSLUCENT:
                    Core::GetEngine().GetGL()->Enable(GL_BLEND);
                    Core::GetEngine().GetGL()->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    cmd.mat->SetScalarParameter("masked", false);
                    break;

                case MASKED:
                    cmd.mat->SetScalarParameter("masked", true);
                    Core::GetEngine().GetGL()->Disable(GL_BLEND);
                    break;

                case OPAQUE:
                    cmd.mat->SetScalarParameter("masked", false);
                    Core::GetEngine().GetGL()->Disable(GL_BLEND);
                    break;
                    
                default:
                    break;
            }

            if(cmd.mat->GetScalarParameter<bool>("useEnvReflections", false)){
                Core::GetEngine().GetLevelManager()->GetLevelAt(0)->skybox->Bind(cmd.mat);
            }
            else {
                Core::GetEngine().GetLevelManager()->GetLevelAt(0)->skybox->BindEmpty(cmd.mat);
            }

            cmd.mat->SetScalarParameter("projection", Core::GetEngine().GetCameraManager()->GetActiveCamera()->GetProjection());
            cmd.mat->SetScalarParameter("view", Core::GetEngine().GetCameraManager()->GetActiveCamera()->GetView());
            cmd.mat->SetScalarParameter("model", cmd.tr->GetTransformMatrix());
            cmd.mat->SetScalarParameter("lightNB", lightMan->GetLightsCount());
            cmd.mat->SetScalarParameter("camPos", Core::GetEngine().GetCameraManager()->GetActiveCamera()->parent->transform->GetPosition());
            cmd.mat->SetScalarParameter("ambientIntensity", Core::GetEngine().GetLevelManager()->GetLevelAt(0)->ambientIntensity);
            cmd.mat->Use();
            Core::GetEngine().GetGL()->BindVertexArray(cmd.VAO);
            Core::GetEngine().GetGL()->PolygonMode(GL_FRONT_AND_BACK, cmd.fillMode);
            Core::GetEngine().GetGL()->DrawElements(GL_TRIANGLES, cmd.indexCount, GL_UNSIGNED_INT, (void*)(cmd.indexOffset * sizeof(uint32_t)));
            cmd.mat->StopUsing();
        }

        if(Renderer::settings.showDebugShapes){
            // --- Debug Physics Shapes ---
            defaultShader->Activate();
            defaultShader->setMat4("projection", Core::GetEngine().GetCameraManager()->GetActiveCamera()->GetProjection());
            defaultShader->setMat4("view", Core::GetEngine().GetCameraManager()->GetActiveCamera()->GetView());

            for (auto& [id,physicBody] : Core::GetEngine().GetLevelManager()->GetLevelAt(0)->physicsBodies) {

                glm::mat4 model = physicBody->parent->transform->GetTransformMatrix();

                defaultShader->setMat4("model", model);

                DebugShape* shape = physicBody->GetDebugShape();

                if(shape){
                    Core::GetEngine().GetGL()->BindVertexArray(shape->GetVAO());
                    Core::GetEngine().GetGL()->DrawElements(GL_LINES, shape->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
                }
                else{
                    DEBUG_ERROR("Debug shape wasn't initialized for physics body of actor : "+physicBody->parent->GetName());
                }
            }
        }
        
        Core::GetEngine().GetGL()->BindVertexArray(0);
        Core::GetEngine().GetGL()->UseProgram(0);

        auto level = Core::GetEngine().GetLevelManager()->GetLevelAt(0);
        if(level){
            if(level->skybox){
                level->skybox->Draw(Core::GetEngine().GetCameraManager()->GetActiveCamera()->GetView(), Core::GetEngine().GetCameraManager()->GetActiveCamera()->GetProjection());
            }
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
        
        if(!viewportBuffer || !tempBuffer){
            DEBUG_FATAL("Called \"RescaleFramebuffers\" with null viewportBuffer or null tempBuffer");
            return;
        }

        viewportBuffer->RescaleFrameBuffer(newWidth, newHeight);
        tempBuffer->RescaleFrameBuffer(newWidth, newHeight);

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

    void Renderer::ExecuteRenderPasses()
    {
        auto* gl = Core::GetEngine().GetGL();

        for (const auto& pass : renderPasses)
        {
            switch (pass.stage)
            {
                // ---------------------------------------------------------
                // UI — rendered to default framebuffer or already drawn viewport
                // ---------------------------------------------------------
                case RenderStage::UI:
                {
                    pass.callback();
                    break;
                }

                // ---------------------------------------------------------
                // POST PROCESSING
                // ---------------------------------------------------------
                case RenderStage::PostProcess:
                {
                    if (!settings.enablePostProcessing)
                        break;

                    if (!pass.target)
                    {
                        DEBUG_ERROR("PostProcess pass missing target framebuffer");
                        break;
                    }

                    // Render the postprocess pass into pass.target
                    pass.target->Bind();
                    gl->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    pass.callback();
                    pass.target->Unbind();

                    // Resolve viewport MSAA before sampling it
                    if (viewportBuffer->isMultisampled)
                        viewportBuffer->Resolve();

                    GLuint viewportTex = viewportBuffer->GetFrameTexture();
                    GLuint passTex = pass.target->GetFrameTexture();

                    // If blending into viewport
                    if (pass.appendToViewport)
                    {
                        // Blend into tempBuffer
                        tempBuffer->Bind();
                        gl->Clear(GL_COLOR_BUFFER_BIT);

                        blendShader->Activate();
                        blendShader->setInt("blendMode", (int)pass.blendMode);
                        blendShader->setInt("texA", 0); // viewport
                        blendShader->setInt("texB", 1); // postprocess output

                        gl->ActiveTexture(GL_TEXTURE0);
                        gl->BindTexture(GL_TEXTURE_2D, viewportTex);

                        gl->ActiveTexture(GL_TEXTURE1);
                        gl->BindTexture(GL_TEXTURE_2D, passTex);

                        gl->Disable(GL_DEPTH_TEST);
                        gl->BindVertexArray(rectVAO);
                        gl->DrawArrays(GL_TRIANGLES, 0, 6);

                        gl->BindVertexArray(0);
                        blendShader->Deactivate();
                        tempBuffer->Unbind();

                        // Write blended result back into the viewport MSAA FBO
                        viewportBuffer->Bind();
                        gl->Clear(GL_COLOR_BUFFER_BIT);
                        tempBuffer->Draw(rectVAO);  // resolves itself
                        viewportBuffer->Unbind();
                    }

                    break;
                }

                // ---------------------------------------------------------
                // SCENE PASS
                // ---------------------------------------------------------
                case RenderStage::Scene:
                default:
                {
                    if (!pass.target)
                    {
                        DEBUG_ERROR("Scene pass missing target framebuffer");
                        break;
                    }

                    // Render scene into pass.target (maybe MSAA)
                    pass.target->Bind();
                    gl->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    pass.callback();
                    pass.target->Unbind();

                    // Resolve MSAA scene FBO if needed
                    if (pass.target->isMultisampled)
                        pass.target->Resolve();

                    // Draw scene result to screen/UI target
                    pass.target->Draw(rectVAO);

                    // Append to viewport?
                    if (pass.appendToViewport)
                    {
                        // Resolve viewport before sampling it
                        if (viewportBuffer->isMultisampled)
                            viewportBuffer->Resolve();

                        GLuint viewportTex = viewportBuffer->GetFrameTexture();
                        GLuint sceneTex = pass.target->GetFrameTexture();

                        // ---- Blend to temp ----
                        tempBuffer->Bind();
                        gl->Clear(GL_COLOR_BUFFER_BIT);

                        blendShader->Activate();
                        blendShader->setInt("blendMode", (int)pass.blendMode);
                        blendShader->setInt("texA", 0);
                        blendShader->setInt("texB", 1);

                        gl->ActiveTexture(GL_TEXTURE0);
                        gl->BindTexture(GL_TEXTURE_2D, viewportTex);

                        gl->ActiveTexture(GL_TEXTURE1);
                        gl->BindTexture(GL_TEXTURE_2D, sceneTex);

                        gl->Disable(GL_DEPTH_TEST);
                        gl->BindVertexArray(rectVAO);
                        gl->DrawArrays(GL_TRIANGLES, 0, 6);

                        gl->BindVertexArray(0);
                        blendShader->Deactivate();
                        tempBuffer->Unbind();

                        // Write temp back to viewport MSAA
                        viewportBuffer->Bind();
                        gl->Clear(GL_COLOR_BUFFER_BIT);
                        tempBuffer->Draw(rectVAO);
                        viewportBuffer->Unbind();
                    }

                    break;
                }
            }
        }
    }
}