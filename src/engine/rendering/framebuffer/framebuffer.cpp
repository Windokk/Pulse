#include "framebuffer.hpp"

#include "engine/core/engine.hpp"

#include "engine/rendering/backends/opengl/framebuffer/gl_framebuffer.hpp"

namespace Pulse::Engine::Rendering{
    std::shared_ptr<Framebuffer> Framebuffer::Create(const FramebufferSpecifications& spec)
    {
        switch(Core::GetEngine().GetRenderer()->GetRendererAPI())
        {
            case RendererAPI::API::OpenGL:
                return std::make_shared<GLFramebuffer>(spec);

            /*case RendererAPI::API::Vulkan:
                return std::make_shared<VKFramebuffer>(spec);

            case RendererAPI::API::DX11:
                return std::make_shared<DX11Framebuffer>(spec);

            case RendererAPI::API::DX12:
                return std::make_shared<DX12Framebuffer>(spec);*/

            default:
                return nullptr;
        }
    }
}