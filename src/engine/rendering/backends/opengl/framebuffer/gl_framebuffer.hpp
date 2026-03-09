#pragma once

#include "engine/rendering/framebuffer/framebuffer.hpp"

namespace Pulse::Engine::Rendering{

    class GLFramebuffer : public Framebuffer{
        public:
            void CheckFBStatus();

            GLFramebuffer(const FramebufferSpecifications &spec);
            virtual ~GLFramebuffer();

            void Bind() override;
            void Unbind() override;

            void ResolveMultisampled() override;

            void Resize(uint32_t width, uint32_t height) override;

            uint32_t GetColorAttachment() const override;

        private:
            uint32_t m_FBO = 0;
            uint32_t m_RBO = 0;
            uint32_t m_ColorAttachment = 0;

            uint32_t m_ResolveFBO = 0;
            uint32_t m_ResolveTexture = 0;
    };

}