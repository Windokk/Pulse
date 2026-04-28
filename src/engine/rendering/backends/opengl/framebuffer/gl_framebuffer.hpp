#pragma once

#include "engine/rendering/framebuffer/framebuffer.hpp"

namespace Pulse::Engine::Rendering{

    class GLFramebuffer : public Framebuffer{
        public:
            void CheckFBStatus();

            GLFramebuffer(const FramebufferSpecifications &spec);
            void Destroy() override;

            void Bind() override;
            void Unbind() override;

            void CopyFrom(std::shared_ptr<Framebuffer> src) override;

            void ResolveMultisampled() override;

            void AttachCubemapArray(uint32_t texture) override;

            void DetachCubemapArray() override;

            uint32_t GetHandle() const override;

            void Resize(uint32_t width, uint32_t height) override;

            bool IsValid() const override;

            uint32_t GetColorAttachment() const override;
            uint32_t GetDepthAttachment() const override;
            
            uint32_t GetResolveColorAttachment() const override;
            uint32_t GetResolveDepthAttachment() const override;

        private:
            uint32_t m_FBO = 0;
            uint32_t m_ColorAttachment = 0;
            uint32_t m_DepthAttachment = 0;

            uint32_t m_ResolveFBO = 0;
            uint32_t m_ResolveColorAttachment = 0;
            uint32_t m_ResolveDepthAttachment = 0;
    };

}