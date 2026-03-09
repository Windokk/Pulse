#pragma once

#include <vector>
#include <memory>
#include <limits>

#include "engine/rendering/utils.hpp"

#include "engine/filesystem/filesystem.hpp"

#include <ufbx/ufbx.h>

namespace Pulse::Engine::Rendering {

    struct SubMesh {
        size_t indexOffset;
        size_t indexCount;
        size_t verticesCount;
    };

    class Material;
    class CommandBuffer;
    class Mesh;

    struct DrawCommand
    {
        uint32_t indexOffset;
        uint32_t indexCount;

        Mesh* mesh;
        Material* material;
        ECS::Components::Transform* transform;

        glm::vec3 boundsMin;
        glm::vec3 boundsMax;

        uint32_t objectID;

        glm::vec3 min;
        glm::vec3 max;
    };

    class Mesh
    {
        public:

            virtual void Create(std::vector<Vertex> vertices, std::vector<uint32_t> indices) = 0;

            virtual void CreateFromFBX(const ufbx_mesh *ufbx_mesh, double scene_unit_meters, 
                ufbx_material_list& ufbx_mats, ufbx_node* mesh_node, COL_RGBA vertexColor = COL_RGBA(0.99f,0.06f,0.75f,1.0f)) = 0;

            virtual ~Mesh() = default;

            const int SubMeshesCount() const { return m_Submeshes.size(); }
            const std::vector<SubMesh>& GetSubMeshes() const { return m_Submeshes; }

            const size_t GetIndexCount() const { return m_Indices.size(); }
            const std::vector<uint32_t>& GetIndices() const { return m_Indices; }

            const size_t GetVertexCount() const { return m_Vertices.size(); }
            const std::vector<Vertex>& GetVertices() const { return m_Vertices; }

            std::vector<DrawCommand> Mesh::CreateDrawCommands(std::shared_ptr<ECS::Components::Transform> tr, int objectID, std::vector<std::shared_ptr<Material>> mats)
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

                    cmd.mesh        = this;
                    cmd.material    = mats[i].get();
                    cmd.transform   = tr.get();
                    cmd.objectID    = objectID;

                    /// @todo Per draw cmd bounds
                    cmd.boundsMax = m_BoundsMax;
                    cmd.boundsMin = m_BoundsMin;

                    cmds.push_back(cmd);
                }

                return cmds;
            }

            void SetAssetID(Filesystem::AssetID assetID) {
                m_AssetID = assetID;
            }

            Filesystem::AssetID GetAssetID() {
                return m_AssetID;
            }

        protected:

            Filesystem::AssetID m_AssetID;

            std::vector<SubMesh> m_Submeshes;
            std::vector<Vertex> m_Vertices;
            std::vector<uint32_t> m_Indices;

            glm::vec3 m_BoundsMin = glm::vec3(std::numeric_limits<float>::max());
            glm::vec3 m_BoundsMax = glm::vec3(std::numeric_limits<float>::lowest());

        public:

            int m_MaterialsSlots = 0;
    };
}
