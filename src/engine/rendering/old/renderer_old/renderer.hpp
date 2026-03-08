#pragma once

#include "engine/rendering/material/material.hpp"
#include "engine/rendering/framebuffer/framebuffer.hpp"
#include "engine/rendering/light/light_manager.hpp"
#include "engine/ecs/components/rendering/model_component.hpp"
#include "engine/rendering/debug/debug.hpp"
#include "engine/rendering/shadow/shadow_manager.hpp"
#include "engine/debugging/logger.hpp"
#include "engine/rendering/camera/camera_manager.hpp"
#include "engine/rendering/texture/cubemap.hpp"

namespace Pulse::Engine{

    namespace ECS::Components{
        class Transform;
    }

    namespace Rendering {

        struct RendererSettings{
            bool showDebugShapes = true;
            bool enableShadows = true;
            bool enablePostProcessing = true;
            int viewportWidth = 0;
            int viewportHeight = 0;
            bool fullscreen = false;   
        };

        enum class RenderPassType {
            Scene,
            Fullscreen,
            Custom
        };

        enum class BlendMode{
            Add,
            Multiply,
            Screen,
            Normal
        };

        struct RenderPass {
            RenderPassType stage;
            std::function<void()> callback;
            std::shared_ptr<FrameBuffer> target = nullptr;
            bool appendToViewport = true;
            BlendMode blendMode = BlendMode::Normal;
            
        };

        class Renderer{
            public:

                void Init(RendererSettings settings = {});
                void Shutdown();
                void Render();

                void SubmitCommand(DrawCommand cmd, bool replace);
                void SubmitCommands(std::vector<DrawCommand> cmds, bool replace);

                void RemoveCommand(DrawCommand cmd);
                void RemoveCommands(std::vector<DrawCommand> cmds);
                std::vector<DrawCommand>* GetDrawList() { return &drawList; }

                void ReorderDrawList();

                void DrawScene();

                void RescaleFramebuffers(int newWidth, int newHeight);

                void AddRenderPass(
                    RenderPassType stage, 
                    std::function<void()> callback, 
                    std::shared_ptr<FrameBuffer> fb = nullptr, 
                    bool appendToViewport = true,
                    BlendMode blendMode = BlendMode::Normal);

                
                void ExecuteRenderPasses();

                unsigned int GetViewportTextureID() { return viewportBuffer->GetFrameTexture(); }

                LightManager* lightMan;

                ShadowManager* shadowMan;

                bool initialized = false;


                GLint maxTextures;
                
                std::shared_ptr<Shader> defaultUnlitShader;
                std::shared_ptr<Cubemap> defaultCubemap;
                std::shared_ptr<Texture> defaultTexture;

            private:

                void CreateRectGeometry();

                void BeginFrame();

                std::vector<DrawCommand> drawList;

                unsigned int rectVAO, rectVBO;
                
                std::vector<RenderPass> renderPasses;

                std::shared_ptr<Shader> blendShader;

                std::shared_ptr<FrameBuffer> viewportBuffer;
                std::shared_ptr<FrameBuffer> tempBuffer;

                std::shared_ptr<Shader> framebufferShader;

                std::shared_ptr<FrameBuffer> currentFramebuffer = nullptr;


                RendererSettings settings;
        };


    }
}