#include "mesh.hpp"

#include "engine/rendering/backends/opengl/mesh/gl_mesh.hpp"
#include "engine/rendering/renderer/renderer.hpp"
#include "engine/core/engine.hpp"

#include "engine/ecs/objects/actors/actor.hpp"

namespace Pulse::Engine::Rendering{
    
    std::shared_ptr<Mesh> Mesh::Create()
    {
        switch(Core::GetEngine().GetRenderer()->GetRendererAPI()->GetAPI())
        {
            case RendererAPI::API::OpenGL:
                return std::make_shared<GLMesh>();

            /*case RendererAPI::API::Vulkan:
                return std::make_shared<VKMesh>();

            case RendererAPI::API::DX11:
                return std::make_shared<DX11Mesh>();

            case RendererAPI::API::DX12:
                return std::make_shared<DX12Mesh>();*/

            default:
                return nullptr;
        }
    }

    std::vector<DrawCommand> Mesh::CreateDrawCommands(std::shared_ptr<ECS::Components::Transform> tr, int modelID, std::vector<std::shared_ptr<Material>> mats)
    {
        if(mats.size() != m_Submeshes.size() && m_Submeshes.size() != 1){
            DEBUG_ERROR("Cannot create draw command for meshes with different submeshes and materials count");
        }

        std::vector<DrawCommand> cmds;

        for (size_t i = 0; i < m_Submeshes.size(); i++)
        {
            DrawCommand cmd;

            cmd.indexOffset = m_Submeshes[i].indexOffset;
            cmd.indexCount  = m_Submeshes[i].indexCount;
            cmd.vertexCount = m_Submeshes[i].vertexCount;

            cmd.mesh        = shared_from_this();
            cmd.material    = mats[i];
            cmd.modelMatrix = tr->GetTransformMatrix();
            cmd.objectID    = tr->parent->GetID().GetAsInt();
            cmd.modelID     = modelID;

            /// @todo Per draw cmd bounds
            cmd.boundsMax = m_BoundsMax;
            cmd.boundsMin = m_BoundsMin;

            cmds.push_back(cmd);
        }

        return cmds;
    }
}