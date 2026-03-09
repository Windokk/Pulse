#include "cubemap.hpp"

#include "engine/core/engine.hpp"

#include "engine/rendering/backends/opengl/texture/cubemap/gl_cubemap.hpp"

#include <stb/stb_image.h>

namespace Pulse::Engine::Rendering{
    std::shared_ptr<Cubemap> Cubemap::Create(
        const TextureSpecifications& spec,
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

    std::shared_ptr<Cubemap> Create(
        const TextureSpecifications& spec,
        const std::vector<Filesystem::Path> imageFiles)
    {
        
        stbi_set_flip_vertically_on_load(false);

        std::array<void*,6>& faces = {nullptr,nullptr,nullptr,nullptr,nullptr,nullptr};

        switch(Core::GetEngine().GetRenderer()->GetRendererAPI())
        {
            case RendererAPI::API::OpenGL:
                return std::make_shared<GLEnvironmentMap>(spec, faces);

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

    std::shared_ptr<Cubemap> Create(
        const TextureSpecifications& spec,
        const Filesystem::Path hdrFile)
    {
        switch(Core::GetEngine().GetRenderer()->GetRendererAPI())
        {
            case RendererAPI::API::OpenGL:
                return std::make_shared<GLEnvironmentMap>(spec, faces);

            /*case API::Vulkan:
                return std::make_shared<VK>(spec, faces);

            case API::DX11:
                return std::make_shared<DX11>(spec, faces);

            case API::DX12:
                return std::make_shared<DX12>(spec, faces);*/

            default:
                return nullptr;
        }
    }
}