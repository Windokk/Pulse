#pragma once

#include <vector>
#include <memory>
#include <limits>

#include "engine/rendering/utils.hpp"

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
        Transform* transform;

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

            virtual int SubMeshesCount() const = 0;

            virtual const std::vector<SubMesh>& GetSubMeshes() const = 0;

            virtual size_t GetIndexCount() const = 0;

            std::vector<DrawCommand> Mesh::CreateDrawCmds(
                std::shared_ptr<ECS::Components::Transform> tr,
                int objectID,
                std::vector<std::shared_ptr<Material>> mats)
            {
                std::vector<DrawCommand> cmds;

                for (size_t i = 0; i < submeshes.size(); i++)
                {
                    DrawCommand cmd;

                    cmd.indexOffset = submeshes[i].indexOffset;
                    cmd.indexCount  = submeshes[i].indexCount;

                    cmd.mesh        = this;
                    cmd.material    = mats[i].get();
                    cmd.transform   = tr.get();
                    cmd.objectID    = objectID;

                    cmd.boundsMax = boundsMax;
                    cmd.boundsMin = boundsMin;

                    cmds.push_back(cmd);
                }

                return cmds;
            }

            void SetAssetID(Filesystem::AssetID assetID) {
                this->assetID = assetID;
            }

            Filesystem::AssetID GetAssetID() {
                return assetID;
            }

        protected:

            Filesystem::AssetID assetID;

            std::vector<SubMesh> submeshes;

            glm::vec3 boundsMin = glm::vec3(std::numeric_limits<float>::max());
            glm::vec3 boundsMax = glm::vec3(std::numeric_limits<float>::lowest());

        public:

            int materialsSlots = 0;
    };
}
