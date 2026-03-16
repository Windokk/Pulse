#pragma once

#include <vector>
#include <memory>
#include <limits>

#include "engine/rendering/utils.hpp"

#include "engine/filesystem/filesystem.hpp"

#include "engine/ecs/components/misc/transform.hpp"

#include <ufbx/ufbx.h>

namespace Pulse::Engine::Rendering {

    struct SubMesh {
        size_t indexOffset;
        size_t indexCount;
        size_t vertexCount;
    };

    class Material;
    class CommandBuffer;
    class Mesh;
    class Pipeline;
    class VertexLayout;

    struct DrawCommand
    {
        uint32_t indexOffset;
        uint32_t indexCount;
        uint32_t vertexCount;

        Mesh* mesh;
        Material* material;
        glm::mat4 modelMatrix;

        glm::vec3 boundsMin;
        glm::vec3 boundsMax;

        uint32_t objectID;
        uint32_t modelID;

        bool fullscreenTri = false;
        
        /// @note Automatically filled by the renderer. Any content will be overriden.
        uint64_t sortKey = 0;
        /// @note Automatically filled by the renderer. Any content will be overriden.
        uint64_t commandID;
    };

    class Mesh
    {
        public:

            static std::shared_ptr<Mesh> Create();

            virtual void Create(std::vector<Vertex> vertices, std::vector<uint32_t> indices, const VertexLayout& layout) = 0;

            virtual void CreateFromFBX(const ufbx_mesh *ufbx_mesh, double scene_unit_meters, 
                ufbx_material_list& ufbx_mats, ufbx_node* mesh_node, const VertexLayout& layout, COL_RGBA vertexColor = COL_RGBA(0.99f,0.06f,0.75f,1.0f)) = 0;

            virtual ~Mesh() = default;

            const int SubMeshesCount() const { return m_Submeshes.size(); }
            const std::vector<SubMesh>& GetSubMeshes() const { return m_Submeshes; }

            const size_t GetIndexCount() const { return m_Indices.size(); }
            const std::vector<uint32_t>& GetIndices() const { return m_Indices; }

            const size_t GetVertexCount() const { return m_Vertices.size(); }
            const std::vector<Vertex>& GetVertices() const { return m_Vertices; }

            std::vector<DrawCommand> CreateDrawCommands(std::shared_ptr<ECS::Components::Transform> tr, int modelID, std::vector<std::shared_ptr<Material>> mats);

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
