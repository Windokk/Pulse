#include "gl_mesh.hpp"

#include "engine/rendering/backends/opengl/gl_utils.hpp"

#include "engine/rendering/shader/shader.hpp"

#include <glm/gtx/string_cast.hpp>

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
    
    static GLenum ShaderDataTypeToOpenGL(ShaderDataType type)
    {
        switch(type)
        {
            case ShaderDataType::Bool:  
            case ShaderDataType::Int:   return GL_INT;

            case ShaderDataType::Float:
            case ShaderDataType::Vec2:
            case ShaderDataType::Vec3:
            case ShaderDataType::Vec4:
            case ShaderDataType::Mat2:
            case ShaderDataType::Mat3:
            case ShaderDataType::Mat4:
                return GL_FLOAT;

            default:
                return GL_FLOAT;
        }
    }

    void GLMesh::Create(std::vector<uint8_t> vertices, std::vector<uint32_t> indices, const VertexLayout& layout)
    {
        m_Vertices.clear();
        m_Submeshes.clear();
        m_Indices.clear();

        m_Vertices = vertices;
        m_Indices = indices;
        m_Submeshes.push_back({
            0,
            indices.size(),
            vertices.size() / layout.GetStride()
        });

        m_VertexCount = vertices.size() / layout.GetStride();

        m_VertexLayout = layout;

        GenerateGLBuffers();
    }

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
    
    void GLMesh::CreateFromFBX(const ufbx_mesh *ufbx_mesh, double scene_unit_meters, ufbx_material_list &ufbx_mats, ufbx_node *mesh_node, COL_RGBA vertexColor)
    {
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

                    m_BoundsMin = glm::min(m_BoundsMin, v.position);
                    m_BoundsMax = glm::max(m_BoundsMax, v.position);

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

                    // Tangent
                    /*if (ufbx_mesh->vertex_tangent.exists) {
                        ufbx_vec3 tangent = ufbx_get_vertex_vec3(&ufbx_mesh->vertex_tangent, vertex_index);
                        v.tangent = { tangent.x, tangent.y, tangent.z };
                    }*/

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

        // Merge all material groups into one VBO/EBO
        m_Vertices.clear();
        m_Submeshes.clear();
        m_Indices.clear();

        uint32_t totalVertexCount = 0;

        for (ufbx_material* mat : materialOrder) {
            auto& group = materialGroups[mat];
        
            ComputeTangents(group.verts, group.localIndices);

            size_t indexOffset = m_Indices.size();
            size_t indexCount = group.localIndices.size();

            size_t vertexOffset = totalVertexCount;
            totalVertexCount += group.verts.size();

            size_t byteOffset = m_Vertices.size();
            size_t byteSize   = group.verts.size() * sizeof(Vertex);

            m_Vertices.resize(byteOffset + byteSize);

            memcpy(
                m_Vertices.data() + byteOffset,
                group.verts.data(),
                byteSize
            );

            for (auto idx : group.localIndices)
                m_Indices.push_back(static_cast<uint32_t>(vertexOffset + idx));

            m_Submeshes.push_back(SubMesh{
                .indexOffset = indexOffset,
                .indexCount = indexCount,
                .vertexCount = group.verts.size()
            });
        }

        m_VertexLayout = {
            {{"aPos",     ShaderDataType::Vec3, 0, offsetof(Vertex, position)},
            {"aTexCoord",ShaderDataType::Vec2, 1, offsetof(Vertex, texCoord)},
            {"aNormal",  ShaderDataType::Vec3, 2, offsetof(Vertex, normal)},
            {"aColor",   ShaderDataType::Vec4, 3, offsetof(Vertex, color)},
            {"aTangent", ShaderDataType::Vec3, 4, offsetof(Vertex, tangent)}}, sizeof(Vertex)
        };

        m_VertexCount = totalVertexCount;

        GenerateGLBuffers();
    }

    GLMesh::~GLMesh()
    {
        glDeleteVertexArrays(1, &m_VAO);
        glDeleteBuffers(1, &m_VBO);
        glDeleteBuffers(1, &m_EBO);
    }

    void GLMesh::GenerateGLBuffers()
    {
        GLint previousVAO = 0;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVAO);

        if (m_VAO)
        {
            glDeleteVertexArrays(1, &m_VAO);
            glDeleteBuffers(1, &m_VBO);
            glDeleteBuffers(1, &m_EBO);

            m_VAO = 0;
            m_VBO = 0;
            m_EBO = 0;
        }

        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);
        glGenBuffers(1, &m_EBO);

        glBindVertexArray(m_VAO);

        // Upload vertex data
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(
            GL_ARRAY_BUFFER,
            m_Vertices.size(),
            m_Vertices.data(),
            GL_STATIC_DRAW
        );

        // Upload index data
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            m_Indices.size() * sizeof(uint32_t),
            m_Indices.data(),
            GL_STATIC_DRAW
        );

        const auto& elements = m_VertexLayout.GetElements();
        uint32_t stride = m_VertexLayout.GetStride();

        for (const auto& element : elements)
        {
            glEnableVertexAttribArray(element.location);

            GLenum glType = GL_FLOAT;

            glType = ShaderDataTypeToOpenGL(element.type);

            glVertexAttribPointer(
                element.location,
                ShaderDataTypeComponentCount(element.type),
                glType,
                GL_FALSE,
                stride,
                (const void*)(uintptr_t)element.offset
            );

        }

        glBindVertexArray(previousVAO);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}