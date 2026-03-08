#pragma once

#include "engine/rendering/renderer/renderer_api.hpp"

#include "engine/rendering/framebuffer/framebuffer.hpp"

namespace Pulse::Engine::Rendering {

    enum class BlendMode{
        Add,
        Multiply,
        Screen,
        Normal
    };

    struct RenderPass{
        std::shared_ptr<Framebuffer> target = nullptr;
        BlendMode blendMode = BlendMode::Normal;
        bool clearColor = true;
        bool clearDepth = true;
    };

    class Renderer{
        public:

            void Init();
            void Render();
            void Shutdown();

            const RendererAPI::API GetRendererAPI() const { if(rendererAPI) return rendererAPI->GetAPI(); }

        private:

            void BeginFrame();
            void EndFrame();

            void BeginRenderPass(const RenderPass& pass);
            void EndRenderPass();

            RendererAPI* rendererAPI;
    };

}