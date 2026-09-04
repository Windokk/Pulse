#include "mesh.hpp"

#include "engine/rendering/backends/opengl/mesh/gl_mesh.hpp"
#include "engine/rendering/renderer/renderer.hpp"
#include "engine/core/engine.hpp"

#include "engine/objects/actors/actor.hpp"

#include <unordered_map>
#include <cstring>
#include <glm/gtc/epsilon.hpp>

namespace {

    struct Vertex {
        glm::vec3 position;
        glm::vec2 texCoord;
        glm::vec3 normal;
        glm::vec4 color;
        glm::vec3 tangent;

        bool operator==(const Vertex& other) const {
            const float eps = 0.0001f;
            return glm::all(glm::epsilonEqual(position, other.position, eps)) &&
                glm::all(glm::epsilonEqual(normal, other.normal, eps)) &&
                glm::all(glm::epsilonEqual(color, other.color, eps)) &&
                glm::all(glm::epsilonEqual(texCoord, other.texCoord, eps))&&
                glm::all(glm::epsilonEqual(tangent, other.tangent, eps));
        }
    };

    inline int Quantize(float v) {
        return static_cast<int>(v * 10000.0f); // match epsilon ~0.0001
    }

    inline void HashCombine(std::size_t& seed, std::size_t hash)
    {
        seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
}

namespace std {
    template <>
    struct hash<Vertex> {
        size_t operator()(const Vertex& v) const {
            size_t seed = 0;

            auto h = [&](float f) {
                return std::hash<int>()(Quantize(f));
            };

            HashCombine(seed, h(v.position.x));
            HashCombine(seed, h(v.position.y));
            HashCombine(seed, h(v.position.z));

            HashCombine(seed, h(v.normal.x));
            HashCombine(seed, h(v.normal.y));
            HashCombine(seed, h(v.normal.z));

            HashCombine(seed, h(v.texCoord.x));
            HashCombine(seed, h(v.texCoord.y));

            HashCombine(seed, h(v.color.r));
            HashCombine(seed, h(v.color.g));
            HashCombine(seed, h(v.color.b));
            HashCombine(seed, h(v.color.a));

            HashCombine(seed, h(v.tangent.x));
            HashCombine(seed, h(v.tangent.y));
            HashCombine(seed, h(v.tangent.z));

            return seed;
        }
    };
}

namespace Pulse::Engine::Rendering{

    namespace {

        void ComputeTangents(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
        {
            // Initialize tangent accumulators
            for (auto &v : vertices) {
                v.tangent = glm::vec3(0.0f);
            }

            // For each triangle:
            for (size_t i = 0; i + 2 < indices.size(); i += 3) {
                Vertex &v0 = vertices[indices[i + 0]];
                Vertex &v1 = vertices[indices[i + 1]];
                Vertex &v2 = vertices[indices[i + 2]];

                glm::vec3 edge1 = v1.position - v0.position;
                glm::vec3 edge2 = v2.position - v0.position;
                glm::vec2 deltaUV1 = v1.texCoord - v0.texCoord;
                glm::vec2 deltaUV2 = v2.texCoord - v0.texCoord;

                float denom = (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
                float f = (denom == 0.0f) ? 0.0f : 1.0f / denom;

                glm::vec3 tangent = glm::vec3(0.0f);
                tangent.x = f * ( deltaUV2.y * edge1.x - deltaUV1.y * edge2.x );
                tangent.y = f * ( deltaUV2.y * edge1.y - deltaUV1.y * edge2.y );
                tangent.z = f * ( deltaUV2.y * edge1.z - deltaUV1.y * edge2.z );

                // Accumulate to each vertex
                v0.tangent += tangent;
                v1.tangent += tangent;
                v2.tangent += tangent;
            }

            // Finally normalize and orthogonalize
            for (auto &v : vertices) {
                // Gram-Schmidt orthogonalization
                glm::vec3 n = v.normal;
                glm::vec3 t = v.tangent;

                // Remove component in normal direction:
                t = glm::normalize(t - n * glm::dot(n, t));

                v.tangent = t;
            }
        }
    }

    MeshCPUData BuildMeshCPUDataFromFBX(const ufbx_mesh *ufbx_mesh, double scene_unit_meters,
        ufbx_material_list &ufbx_mats, ufbx_node *mesh_node, COL_RGBA vertexColor)
    {
        MeshCPUData result;

        double scene_scale = scene_unit_meters;

        struct GroupedTriangles {
            std::vector<Vertex> verts;
            std::vector<unsigned int> localIndices;
            std::unordered_map<Vertex, unsigned int> vertexToIndex;
        };

        std::unordered_map<ufbx_material*, GroupedTriangles> materialGroups;
        std::vector<ufbx_material*> materialOrder;

        for (size_t i = 0; i < ufbx_mesh->num_faces; i++) {
            ufbx_face face = ufbx_mesh->faces.data[i];

            uint32_t mat_index = 0;
            if (ufbx_mesh->face_material.data && i < ufbx_mesh->face_material.count) {
                mat_index = ufbx_mesh->face_material.data[i];
            }

            ufbx_material* mat = nullptr;
            if (ufbx_mesh->materials.data && mat_index < ufbx_mesh->materials.count) {
                mat = ufbx_mesh->materials[mat_index];
            }

            // Fallback to global material list if needed
            if (!mat && ufbx_mats.count > 0 && ufbx_mats.data) {
                mat = ufbx_mats.data[0];
            }

            if (materialGroups.find(mat) == materialGroups.end()) {
                materialOrder.push_back(mat);
            }

            GroupedTriangles& group = materialGroups[mat];
            size_t start = face.index_begin;
            size_t count = face.num_indices;

            // Triangle fan triangulation
            for (size_t j = 1; j + 1 < count; j++) {
                size_t triIndices[3] = {
                    start + 0,
                    start + j,
                    start + j + 1
                };

                for (int k = 0; k < 3; k++) {
                    size_t vertex_index = triIndices[k];
                    Vertex v{};

                    // Position
                    ufbx_vec3 pos = ufbx_get_vertex_vec3(&ufbx_mesh->vertex_position, vertex_index);
                    ufbx_vec3 wpos = ufbx_transform_position(&mesh_node->geometry_to_world, pos);
                    v.position = { wpos.x * scene_scale, wpos.y * scene_scale, wpos.z * scene_scale };

                    result.boundsMin = glm::min(result.boundsMin, v.position);
                    result.boundsMax = glm::max(result.boundsMax, v.position);

                    // Normal
                    if (ufbx_mesh->vertex_normal.exists) {
                        ufbx_vec3 normal = ufbx_get_vertex_vec3(&ufbx_mesh->vertex_normal, vertex_index);
                        ufbx_matrix normal_mtx = ufbx_matrix_for_normals(&mesh_node->geometry_to_world);
                        ufbx_vec3 wn = ufbx_transform_direction(&normal_mtx, normal);
                        v.normal = glm::normalize(glm::vec3{ wn.x, wn.y, wn.z });
                    }

                    // UV
                    if (ufbx_mesh->vertex_uv.exists) {
                        ufbx_vec2 uv = ufbx_get_vertex_vec2(&ufbx_mesh->vertex_uv, vertex_index);
                        v.texCoord = { uv.x, uv.y };
                    }

                    // Color
                    v.color = vertexColor;

                    // Deduplication
                    auto it = group.vertexToIndex.find(v);
                    if (it != group.vertexToIndex.end()) {
                        group.localIndices.push_back(it->second);
                    } else {
                        unsigned int newIndex = static_cast<unsigned int>(group.verts.size());
                        group.verts.push_back(v);
                        group.localIndices.push_back(newIndex);
                        group.vertexToIndex[v] = newIndex;
                    }
                }
            }
        }

        uint32_t totalVertexCount = 0;

        for (ufbx_material* mat : materialOrder) {
            auto& group = materialGroups[mat];

            ComputeTangents(group.verts, group.localIndices);

            size_t indexOffset = result.indices.size();
            size_t indexCount = group.localIndices.size();

            size_t vertexOffset = totalVertexCount;
            totalVertexCount += group.verts.size();

            size_t byteOffset = result.vertices.size();
            size_t byteSize   = group.verts.size() * sizeof(Vertex);

            result.vertices.resize(byteOffset + byteSize);

            memcpy(
                result.vertices.data() + byteOffset,
                group.verts.data(),
                byteSize
            );

            for (auto idx : group.localIndices)
                result.indices.push_back(static_cast<uint32_t>(vertexOffset + idx));

            result.submeshes.push_back(SubMesh{
                .indexOffset = indexOffset,
                .indexCount = indexCount,
                .vertexCount = group.verts.size()
            });
        }

        result.layout = {
            {{"aPos",     ShaderDataType::Vec3, 0, offsetof(Vertex, position)},
            {"aTexCoord",ShaderDataType::Vec2, 1, offsetof(Vertex, texCoord)},
            {"aNormal",  ShaderDataType::Vec3, 2, offsetof(Vertex, normal)},
            {"aColor",   ShaderDataType::Vec4, 3, offsetof(Vertex, color)},
            {"aTangent", ShaderDataType::Vec3, 4, offsetof(Vertex, tangent)}}, sizeof(Vertex)
        };

        result.success = true;

        return result;
    }

    MeshCPUData DecodeMeshFile(const Filesystem::Path &path)
    {
        MeshCPUData result;

        ufbx_load_opts opts = { 0 };
        ufbx_error error;
        const std::string filePath = path.GetNativePath();
        ufbx_scene *scene = ufbx_load_file(filePath.c_str(), &opts, &error);
        if (!scene) {
            DEBUG_ERROR(
                "Failed to load " + path.full + " : " +
                (error.description.data ? error.description.data : "Unknown error"));
            return result;
        }

        if (scene->meshes.count > 1) {
            ufbx_free_scene(scene);
            DEBUG_ERROR("Multiple meshes per fbx file isn't supported yet.");
            return result;
        }

        ufbx_mesh* ufbx_mesh = scene->meshes.data[0];

        ufbx_node* mesh_node = nullptr;

        for (size_t i = 0; i < scene->nodes.count; i++) {
            ufbx_node* node = scene->nodes.data[i];
            if (node->mesh == ufbx_mesh) {
                mesh_node = node;
                break;
            }
        }

        result = BuildMeshCPUDataFromFBX(ufbx_mesh, scene->settings.unit_meters, scene->materials, mesh_node);

        ufbx_free_scene(scene);

        return result;
    }

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

    std::vector<DrawCommand> Mesh::CreateDrawCommands(std::shared_ptr<Objects::Components::Transform> tr, int modelID, std::vector<std::shared_ptr<Material>> mats)
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