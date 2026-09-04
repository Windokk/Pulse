#pragma once

#include <cstdint>
#include <vector>
#include <memory>

#include <glm/glm.hpp>

namespace Pulse::Engine::Levels {
    class Level;
}

namespace Pulse::Engine::Rendering {
    class Mesh;
}

namespace Pulse::Engine::Rendering::Raytracing {

    struct BVHNode;

    // GPU-facing triangle data, split into a "hot" buffer (touched on every BVH traversal step) and a
    // "cold" buffer (touched once, only for the triangle a ray finally hits) - see path_trace.comp for
    // the matching GLSL structs. Both mirror std430 layout exactly (all-vec4 members, no padding traps).
    struct GPUTrianglePos
    {
        glm::vec4 v0, v1, v2; // xyz = world-space position, w unused
    };

    struct GPUTriangleAttrib
    {
        glm::vec4 n0, n1, n2;  // xyz = world-space normal, w = u (texcoord) for that vertex
        glm::vec4 uvMatID;     // x/y/z = v (texcoord) for vertices 0/1/2, w = material index
        glm::vec4 t0, t1, t2;  // xyz = world-space tangent, w unused - only read when a material's
                                // normalTex handle is valid (see path_trace.comp)
    };

    // Texture handles are ARB_bindless_texture handles (see GLTexture2D::GetBindlessHandle), split into
    // two uint32 halves rather than stored as a single uint64_t so the struct doesn't need
    // GL_ARB_gpu_shader_int64 on the GLSL side - path_trace.comp reconstructs a sampler2D directly from
    // the uvec2 via the sampler2D(uvec2) constructor ARB_bindless_texture provides. A handle is only
    // valid (and its bit set in textureFlags) if the material actually has that texture assigned and the
    // driver supports bindless textures - otherwise the corresponding scalar parameter is used as-is.
    struct GPUMaterial
    {
        glm::vec4 albedo   = glm::vec4(1.0f);
        glm::vec4 emissive = glm::vec4(0.0f);
        float roughness = 0.9f;
        float metallic  = 0.0f;
        float ior       = 1.5f;
        uint32_t textureFlags = 0; // bit 0 = albedo, 1 = metallic, 2 = roughness, 3 = normal
        glm::uvec2 albedoTex    = glm::uvec2(0);
        glm::uvec2 metallicTex  = glm::uvec2(0);
        glm::uvec2 roughnessTex = glm::uvec2(0);
        glm::uvec2 normalTex    = glm::uvec2(0);
    };

    enum GPUMaterialTextureBit : uint32_t
    {
        GPUMaterialTexAlbedo    = 1u << 0,
        GPUMaterialTexMetallic  = 1u << 1,
        GPUMaterialTexRoughness = 1u << 2,
        GPUMaterialTexNormal    = 1u << 3,
    };

    struct RaytraceScene
    {
        std::vector<BVHNode> bvhNodes;
        std::vector<GPUTrianglePos> trianglePositions;
        std::vector<GPUTriangleAttrib> triangleAttribs;
        std::vector<GPUMaterial> materials;
    };

    // Per-model data needed to flatten its triangles, captured up front on the main thread so the
    // (much more expensive, O(triangle count)) flatten+BVH-build pass in BuildFromSnapshot() can run
    // on a worker thread without touching any live Level/Actor/Transform/Material state - see
    // SceneBuilder::CaptureSnapshot()'s comment for why that split exists.
    struct ModelSnapshot
    {
        // Meshes are never mutated after creation (see Mesh::Create/CreateFromFBX/CreateFromData, all
        // one-shot at load time), so reading through this shared_ptr from a worker thread while the
        // main thread carries on is safe.
        std::shared_ptr<Mesh> mesh;
        // One already-resolved index into RaytraceSceneSnapshot::materials per submesh.
        std::vector<uint32_t> materialIndicesPerSubmesh;
        glm::mat4 worldMatrix = glm::mat4(1.0f);
    };

    struct RaytraceSceneSnapshot
    {
        std::vector<ModelSnapshot> models;
        // Already extracted (scalar params + bindless texture handles resolved) and deduped by
        // Material* - see SceneBuilder::CaptureSnapshot().
        std::vector<GPUMaterial> materials;
    };

    class SceneBuilder
    {
        public:
            // Walks every active Model component in `level`, flattens their (world-transformed)
            // triangles and materials into GPU-ready arrays, and builds a BVH over them. Materials are
            // read from their scalar parameters (albedo/roughness/metallic/emissive) plus, when present,
            // bindless handles for their albedo/metallic/roughness/normal textures - a texture, when
            // assigned, takes priority over (multiplies, for metallic/roughness) the scalar value. See
            // GPUMaterial's comment for how the handles are packed.
            //
            // Equivalent to BuildFromSnapshot(CaptureSnapshot(level)) - kept as a single call for
            // callers (the offline Raytracer) that want the whole thing done synchronously in one shot.
            static RaytraceScene Build(Levels::Level* level);

            // Main-thread-only, cheap (bounded by model/material count, not triangle count): walks
            // `level`'s live actors/materials and produces a self-contained snapshot with every
            // GL-touching bit (bindless texture handle resolution via Material::GetTextureParameter)
            // already done. Must be called from the thread that owns the GL context.
            static RaytraceSceneSnapshot CaptureSnapshot(Levels::Level* level);

            // Pure CPU (triangle flattening + BVH build) - touches no engine singleton, no GL, no live
            // Level/Actor/Transform state, only the snapshot's copied data - safe to call from any
            // thread, including a background worker while the main thread continues running.
            static RaytraceScene BuildFromSnapshot(const RaytraceSceneSnapshot& snapshot);
    };

}
