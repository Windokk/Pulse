#pragma once

#include "engine/rendering/utils.hpp"

#include "engine/rendering/texture/texture.hpp"

namespace Pulse::Engine::Rendering {

    struct FramebufferSpecifications
    {
        uint32_t width = 0;
        uint32_t height = 0;

        bool multisampled = false;
        uint32_t samplesCount = 4;

        bool hasColor = true;
        bool hasDepth = true;

        TextureSpecifications colorSpecs = []{
            TextureSpecifications s;
            s.format = TextureFormat::RGBA;
            s.internalFormat = TextureInternalFormat::RGBA8;
            return s;
        }();

        TextureSpecifications depthSpecs = []{
            TextureSpecifications s;
            s.format = TextureFormat::Depth;
            s.internalFormat = TextureInternalFormat::Depth24;
            s.minFilter = TextureFilter::Nearest;
            s.magFilter = TextureFilter::Nearest;
            s.wrapS = TextureWrap::ClampEdge;
            s.wrapT = TextureWrap::ClampEdge;
            return s;
        }();
        };

    class Framebuffer
    {
        public:

            virtual void Destroy() = 0;

            virtual void Bind() = 0;
            virtual void Unbind() = 0;

            virtual void ResolveMultisampled() = 0;

            virtual void Resize(uint32_t width, uint32_t height) = 0;

            virtual void CopyFrom(std::shared_ptr<Framebuffer> src) = 0;

            virtual uint32_t GetColorAttachment() const = 0;
            virtual uint32_t GetDepthAttachment() const = 0;
            virtual uint32_t GetResolveColorAttachment() const = 0;
            virtual uint32_t GetResolveDepthAttachment() const = 0;

            virtual uint32_t GetHandle() const = 0;
            
            virtual bool IsValid() const = 0;

            uint32_t GetWidth() const { return m_Specifications.width; }
            uint32_t GetHeight() const { return m_Specifications.height; }

            const FramebufferSpecifications& GetSpecifications() const {return m_Specifications; }

            static std::shared_ptr<Framebuffer> Create(
                const FramebufferSpecifications& specs
            );

        protected:
            
            FramebufferSpecifications m_Specifications;
    };
}