#pragma once

#include "engine/rendering/material/material.hpp"
#include "engine/rendering/framebuffer/framebuffer.hpp"
#include "engine/rendering/light/light_manager.hpp"
#include "engine/ecs/components/rendering/model_component.hpp"
#include "engine/rendering/debug/debug.hpp"
#include "engine/rendering/shadow/shadow_manager.hpp"
#include "engine/debugging/debugger.hpp"
#include "engine/rendering/camera/camera_manager.hpp"

namespace Epoch::Engine{

    namespace ECS::Components{
        class Transform;
    }

    namespace Rendering {

        class Renderer;

        struct RendererSettings{
            bool showDebugShapes = false;
            //TODO : bool enableStatsOverlay = false;
            bool enableShadows = true;
            bool enablePostProcessing = true;
            private:
                int windowWidth = 0;
                int windowHeight = 0;
                bool fullscreen = false;
            friend class Epoch::Engine::Rendering::Renderer;
        };

        enum class RenderStage {
            Scene,
            PostProcess,
            UI
        };

        enum class BlendMode{
            Add,
            Multiply,
            Screen,
            Normal
        };

        struct RenderPass {
            RenderStage stage;
            std::function<void()> callback;
            std::shared_ptr<FrameBuffer> target = nullptr;
            bool appendToViewport = true;
            BlendMode blendMode = BlendMode::Normal;
            
        };

        struct DrawCommand {
            int indexOffset = 0;
            int indexCount = 0;
            unsigned int VAO;
            unsigned int VBO;
            std::shared_ptr<Material> mat;
            std::shared_ptr<ECS::Components::Transform> tr;
            int id;
            int fillMode = GL_FILL;
            glm::vec3 boundsMin = glm::vec3(0);
            glm::vec3 boundsMax = glm::vec3(0);
        };

        class Renderer{
            public:

                void Init(RendererSettings settings = {});
                void InitFramebuffers();
                void Shutdown();
                void Render();

                void SubmitCommand(DrawCommand cmd, bool replace);
                void SubmitCommands(std::vector<DrawCommand> cmds, bool replace);

                void RemoveCommand(DrawCommand cmd);
                void RemoveCommands(std::vector<DrawCommand> cmds);

                void ReorderDrawList();

                void DrawScene();

                void RescaleFramebuffers(int newWidth, int newHeight);

                void AddRenderPass(
                    RenderStage stage, 
                    std::function<void()> callback, 
                    std::shared_ptr<FrameBuffer> fb = nullptr, 
                    bool appendToViewport = true,
                    BlendMode blendMode = BlendMode::Normal);

                
                void ExecuteRenderPasses();

                unsigned int GetViewportTextureID() { return viewportBuffer->GetFrameTexture(); }

                LightManager* lightMan;

                ShadowManager* shadowMan;

                bool initialized = false;

            private:

                void CreateRectGeometry();

                void BeginFrame();

                std::vector<DrawCommand> drawList;

                unsigned int rectVAO, rectVBO;
                
                std::vector<RenderPass> renderPasses;

                std::shared_ptr<Shader> blendShader;
                FrameBuffer* viewportBuffer = nullptr;

                std::shared_ptr<Shader> framebufferShader;

                std::shared_ptr<Shader> unlitShader;

                RendererSettings settings;
        };


    }
}