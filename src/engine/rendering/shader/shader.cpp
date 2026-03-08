#include "shader.hpp"

#include "engine/core/engine.hpp"

namespace Pulse::Engine::Rendering {
    std::shared_ptr<Shader> Shader::Create(const Filesystem::Path &vertexPath, const Filesystem::Path &fragmentPath, const Filesystem::Path &geometryPath)
    {
        switch (Core::GetEngine().GetRenderer()->GetRendererAPI())
        {
            case RendererAPI::API::OpenGL:
                return std::make_shared<GLShader>(vertexPath, fragmentPath, geometryPath);

            /*case RendererAPI::API::Vulkan:
                return std::make_shared<VKShader>(vertexPath, fragmentPath, geometry);

            case RendererAPI::API::DX11:
                return std::make_shared<DX11Shader>(vertexPath, fragmentPath, geometry);

            case RendererAPI::API::DX12:
                return std::make_shared<DX12Shader>(vertexPath, fragmentPath, geometry);*/
        }

        throw std::runtime_error("Unknown RendererAPI");
    }
}