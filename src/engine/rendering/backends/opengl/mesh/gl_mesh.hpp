#pragma once

#include "engine/rendering/mesh/mesh.hpp"

namespace Pulse::Engine::Rendering{

    class GLMesh : public Mesh{
        public:
            void Create(std::vector<Vertex> vertices, std::vector<uint32_t> indices) override;

            void CreateFromFBX(const ufbx_mesh *ufbx_mesh, double scene_unit_meters, 
                ufbx_material_list& ufbx_mats, ufbx_node* mesh_node, 
                COL_RGBA vertexColor = COL_RGBA(0.99f,0.06f,0.75f,1.0f)) override;

            ~GLMesh() override;

            void GenGLBuffers();

        private:
            uint32_t VAO, VBO, EBO = 0;

    };
}