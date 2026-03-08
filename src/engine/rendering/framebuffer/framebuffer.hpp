#pragma once

#include "engine/rendering/utils.hpp"

namespace Pulse::Engine::Rendering {

    struct FramebufferSpecification
    {
        uint32_t Width = 0;
        uint32_t Height = 0;

        bool Multisampled = false;
    };

    class Framebuffer
    {
    public:

        virtual ~Framebuffer() = default;

        virtual void Bind() = 0;
        virtual void Unbind() = 0;

        virtual void Resize(uint32_t width, uint32_t height) = 0;

        virtual uint32_t GetColorAttachment() const = 0;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;

        virtual const FramebufferSpecification& GetSpecification() const = 0;

        static std::shared_ptr<Framebuffer> Create(
            const FramebufferSpecification& spec
        );
    };
}