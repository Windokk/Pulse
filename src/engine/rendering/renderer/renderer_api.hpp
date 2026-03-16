#pragma once

#include <memory>

#include <cstdint>

#include <type_traits>

namespace Pulse::Engine::Rendering {

    class DrawCommand;
    class Mesh;
    class Material;
    class Pipeline;
    class RenderPass;

    enum class ClearBit : uint32_t
    {
        None  = 0,
        Color = 1 << 0,
        Depth = 1 << 1,
        Stencil = 1 << 2
    };

    inline ClearBit operator|(ClearBit a, ClearBit b)
    {
        using T = std::underlying_type_t<ClearBit>;
        return static_cast<ClearBit>(static_cast<T>(a) | static_cast<T>(b));
    }

    inline ClearBit operator&(ClearBit a, ClearBit b)
    {
        using T = std::underlying_type_t<ClearBit>;
        return static_cast<ClearBit>(static_cast<T>(a) & static_cast<T>(b));
    }

    inline ClearBit& operator|=(ClearBit& a, ClearBit b)
    {
        a = a | b;
        return a;
    }

    class RendererAPI{
        public: 
            enum class API : uint32_t
            {
                OpenGL = 0,
                Vulkan,
                DX11,
                DX12
            };

            virtual ~RendererAPI() = default;

            virtual void ExecuteDrawCommand(const DrawCommand& command, const std::shared_ptr<RenderPass> pass) = 0;

            virtual void ToggleMultisampling(const bool on) = 0;

            static std::shared_ptr<RendererAPI> Create(API api);

            const API GetAPI() const { return api; }

            virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;

            virtual void SetClearColor(float r, float g, float b, float a) = 0;

            virtual void Clear(ClearBit clearBits) = 0;

            virtual std::string GetDeviceVendor() = 0;
            virtual std::string GetRendererName() = 0;
            virtual std::string GetDriverVersion() = 0;

        protected:
            API api;
    };

}