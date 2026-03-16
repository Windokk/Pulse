#include "material.hpp"

#include "engine/core/engine.hpp"

#include "engine/rendering/renderer/renderer.hpp"

#include "engine/rendering/backends/opengl/material/gl_material.hpp"

#include "engine/rendering/shader/shader.hpp"
#include "engine/rendering/pipeline/pipeline.hpp"

namespace Pulse::Engine::Rendering{

    std::shared_ptr<Material> Material::Create(std::shared_ptr<Shader> shader, std::shared_ptr<Pipeline> pipeline, bool receivesShadows, Opacity opacity)
    {
        if(shader->GetAssetID() != pipeline->GetSpecifications().shader->GetAssetID()){
            DEBUG_ERROR("Tried creating a material with different shaders. Specified shader and specified pipeline's shader are not the same !");
            return nullptr;
        }

        switch(Core::GetEngine().GetRenderer()->GetRendererAPI()->GetAPI())
        {
            case RendererAPI::API::OpenGL:
                return std::make_shared<GLMaterial>(shader, pipeline, receivesShadows, opacity);

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