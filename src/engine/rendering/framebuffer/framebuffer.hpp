#pragma once

#include "engine/rendering/utils.hpp"

namespace Pulse::Engine::Rendering {

    struct FramebufferSpecifications
    {
        uint32_t width = 0;
        uint32_t height = 0;

        bool multisampled = false;
    };

    class Framebuffer
    {
        public:

            virtual ~Framebuffer() = default;

            virtual void Bind() = 0;
            virtual void Unbind() = 0;

            virtual void ResolveMultisampled() = 0;

            virtual void Resize(uint32_t width, uint32_t height) = 0;

            virtual uint32_t GetColorAttachment() const = 0;

            uint32_t GetWidth() const { return m_Specifications.width; }
            uint32_t GetHeight() const { return m_Specifications.height; }

            const FramebufferSpecifications& GetSpecification() const {return m_Specifications; }

            static std::shared_ptr<Framebuffer> Create(
                const FramebufferSpecifications& spec
            );

        protected:
            
            FramebufferSpecifications m_Specifications;
    };
}