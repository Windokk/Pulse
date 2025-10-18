#pragma once

#include "engine/ecs/components/core/transform.hpp"
#include "engine/filesystem/filesystem.hpp"
#include "engine/rendering/material/material.hpp"

#include <ufbx/ufbx.h>

namespace Epoch::Engine::Rendering{

    struct SubMesh {
        size_t indexOffset;
        size_t indexCount;
    };

    class Mesh {
        public:
            Mesh(const ufbx_mesh *ufbx_mesh, double scene_unit_meters, ufbx_material_list& ufbx_mats, COL_RGBA diffuse = COL_RGBA(0.99f,0.06f,0.75f,1.0f));
            ~Mesh();

            void DrawWithoutMaterial() const;

            std::vector<DrawCommand> CreateDrawCmds(std::shared_ptr<ECS::Components::Transform> tr, int objectID, std::vector<std::shared_ptr<Material>> mats);

            int SubMeshesCount() const { return submeshes.size(); }

            int materialsSlots;
            
            Filesystem::AssetID assetID;

            void SetAssetID(Filesystem::AssetID assetID) {
                this->assetID = assetID;
            }

        private:
            GLuint VAO, VBO, EBO;
            size_t totalIndexCount;
            std::vector<Vertex> vertices;
            std::vector<GLuint> indices;
            std::vector<SubMesh> submeshes;

            bool LoadMesh(const ufbx_mesh *ufbx_mesh, double scene_unit_meters, ufbx_material_list& ufbx_mats, COL_RGBA diffuse);
            

    };
    
}

