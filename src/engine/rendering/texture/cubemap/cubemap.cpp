#include "cubemap.hpp"

#include "engine/core/engine.hpp"

namespace Pulse::Engine::Rendering{
    std::shared_ptr<Cubemap> Cubemap::Create(
        const TextureSpecification& spec,
        const std::array<void*,6>& faces
    )
    {
        switch(Core::GetEngine().GetRenderer()->GetRendererAPI())
        {
            case RendererAPI::API::OpenGL:
                return std::make_shared<GLCubemap>(spec, faces);

            /*case API::Vulkan:
                return std::make_shared<VKCubemap>(spec, faces);

            case API::DX11:
                return std::make_shared<DX11Cubemap>(spec, faces);

            case API::DX12:
                return std::make_shared<DX12Cubemap>(spec, faces);*/

            default:
                return nullptr;
        }
    }
}