#include "pipeline.hpp"

#include "engine/core/engine.hpp"

#include "engine/rendering/shader/shader.hpp"

#include "engine/rendering/backends/opengl/pipeline/gl_pipeline.hpp"

#include "engine/rendering/renderer/renderer.hpp"

namespace Pulse::Engine::Rendering
{
    std::shared_ptr<Pipeline> Pipeline::Create(const PipelineSpecifications &specs)
    {
        switch(Core::GetEngine().GetRenderer()->GetRendererAPI()->GetAPI())
        {
            case RendererAPI::API::OpenGL:
                return std::make_shared<GLPipeline>(specs);

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
    uint32_t VertexElement::Size() const
    {
        return ShaderDataTypeSize(type);
    }
}