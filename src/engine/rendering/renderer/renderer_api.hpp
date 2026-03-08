#pragma once

namespace Pulse::Engine::Rendering {

    struct RenderCommand{

    };

    class RendererAPI{
        public: 
            enum class API
            {
                None = 0,
                OpenGL,
                Vulkan,
                DX11,
                DX12
            };

            virtual ~RendererAPI() = default;

            virtual void Init() = 0;

            virtual void SetViewport(uint32_t x, uint32_t y,
                                    uint32_t width, uint32_t height) = 0;

            virtual void SetClearColor(float r, float g, float b, float a) = 0;

            virtual void Clear() = 0;

            virtual void DrawIndexed(uint32_t indexCount) = 0;

            const API GetAPI() const { return api; }

        protected:
            API api;
    };

}