#include "raytrace_scene.hpp"

#include "engine/rendering/raytracing/bvh.hpp"

#include "engine/levels/level.hpp"

#include "engine/objects/actors/actor.hpp"
#include "engine/objects/components/rendering/model_component.hpp"
#include "engine/objects/components/misc/transform.hpp"

#include "engine/rendering/mesh/mesh.hpp"
#include "engine/rendering/material/material.hpp"
#include "engine/rendering/pipeline/pipeline.hpp"

#include <algorithm>
#include <cstring>
#include <unordered_map>

namespace Pulse::Engine::Rendering::Raytracing {

    namespace {

        bool FindElementOffset(const VertexLayout& layout, const std::string& name, uint32_t& outOffset)
        {
            for (const auto& element : layout.GetElements())
            {
                if (element.name == name)
                {
                    outOffset = element.offset;
                    return true;
                }
            }
            return false;
        }

        glm::vec3 ReadVec3(const uint8_t* base, size_t vertexOffset, uint32_t elementOffset)
        {
            glm::vec3 v;
            std::memcpy(&v, base + vertexOffset + elementOffset, sizeof(glm::vec3));
            return v;
        }

        glm::vec2 ReadVec2(const uint8_t* base, size_t vertexOffset, uint32_t elementOffset)
        {
            glm::vec2 v;
            std::memcpy(&v, base + vertexOffset + elementOffset, sizeof(glm::vec2));
            return v;
        }

        glm::uvec2 PackBindlessHandle(uint64_t handle)
        {
            return glm::uvec2((uint32_t)(handle & 0xFFFFFFFFu), (uint32_t)(handle >> 32));
        }

        // Reads scalar parameters, then (when the material has one bound) a bindless handle for each of
        // the 4 texture slots the raytracer understands - same sampler names the rasterizer's "lit" shader
        // uses, so any material authored for the rasterizer picks up its textures here for free.
        GPUMaterial ExtractMaterial(const std::shared_ptr<Material>& mat)
        {
            GPUMaterial gm;

            if (!mat)
                return gm;

            if (auto v = mat->GetScalarParameter("albedo"))
            {
                if (auto* vec3v = std::get_if<glm::vec3>(&*v))
                    gm.albedo = glm::vec4(*vec3v, 1.0f);
                else if (auto* vec4v = std::get_if<glm::vec4>(&*v))
                    gm.albedo = *vec4v;
            }

            if (auto v = mat->GetScalarParameter("roughness"))
                if (auto* f = std::get_if<float>(&*v))
                    gm.roughness = *f;

            if (auto v = mat->GetScalarParameter("metallic"))
                if (auto* f = std::get_if<float>(&*v))
                    gm.metallic = *f;

            if (auto v = mat->GetScalarParameter("emissive"))
            {
                if (auto* vec3v = std::get_if<glm::vec3>(&*v))
                    gm.emissive = glm::vec4(*vec3v, 1.0f);
                else if (auto* f = std::get_if<float>(&*v))
                    gm.emissive = glm::vec4(*f);
            }

            if (uint64_t h = mat->GetTextureParameter("albedo"))
            {
                gm.albedoTex = PackBindlessHandle(h);
                gm.textureFlags |= GPUMaterialTexAlbedo;
            }

            if (uint64_t h = mat->GetTextureParameter("metallicMap"))
            {
                gm.metallicTex = PackBindlessHandle(h);
                gm.textureFlags |= GPUMaterialTexMetallic;
            }

            if (uint64_t h = mat->GetTextureParameter("roughnessMap"))
            {
                gm.roughnessTex = PackBindlessHandle(h);
                gm.textureFlags |= GPUMaterialTexRoughness;
            }

            if (uint64_t h = mat->GetTextureParameter("normalMap"))
            {
                gm.normalTex = PackBindlessHandle(h);
                gm.textureFlags |= GPUMaterialTexNormal;
            }

            return gm;
        }

    }

    RaytraceScene SceneBuilder::Build(Levels::Level* level)
    {
        return BuildFromSnapshot(CaptureSnapshot(level));
    }

    RaytraceSceneSnapshot SceneBuilder::CaptureSnapshot(Levels::Level* level)
    {
        RaytraceSceneSnapshot snapshot;

        if (!level)
            return snapshot;

        std::unordered_map<Rendering::Material*, uint32_t> materialIndices;

        for (auto& [id, model] : level->models)
        {
            if (!model || !model->Active())
                continue;

            std::shared_ptr<Mesh> mesh = model->GetMesh();
            if (!mesh)
                continue;

            std::vector<std::shared_ptr<Material>> mats = model->GetMaterials();
            if (mats.empty())
                continue;

            const std::vector<SubMesh>& submeshes = mesh->GetSubMeshes();

            ModelSnapshot modelSnap;
            modelSnap.mesh = mesh;
            modelSnap.worldMatrix = model->parent->transform->GetTransformMatrix();
            modelSnap.materialIndicesPerSubmesh.resize(submeshes.size());

            for (size_t submeshIdx = 0; submeshIdx < submeshes.size(); submeshIdx++)
            {
                std::shared_ptr<Material> mat = mats[std::min(submeshIdx, mats.size() - 1)];

                uint32_t materialIndex;
                auto it = materialIndices.find(mat.get());
                if (it != materialIndices.end())
                {
                    materialIndex = it->second;
                }
                else
                {
                    materialIndex = (uint32_t)snapshot.materials.size();
                    snapshot.materials.push_back(ExtractMaterial(mat));
                    materialIndices.emplace(mat.get(), materialIndex);
                }

                modelSnap.materialIndicesPerSubmesh[submeshIdx] = materialIndex;
            }

            snapshot.models.push_back(std::move(modelSnap));
        }

        return snapshot;
    }

    RaytraceScene SceneBuilder::BuildFromSnapshot(const RaytraceSceneSnapshot &snapshot)
    {
        RaytraceScene scene;
        scene.materials = snapshot.materials;

        std::vector<GPUTrianglePos> positions;
        std::vector<GPUTriangleAttrib> attribs;

        for (auto& modelSnap : snapshot.models)
        {
            const Mesh* mesh = modelSnap.mesh.get();
            if (!mesh)
                continue;

            const VertexLayout& layout = mesh->GetVertexLayout();
            uint32_t stride = layout.GetStride();

            uint32_t posOffset = 0, normalOffset = 0, uvOffset = 0, tangentOffset = 0;
            bool hasPos = FindElementOffset(layout, "aPos", posOffset);
            bool hasNormal = FindElementOffset(layout, "aNormal", normalOffset);
            bool hasUV = FindElementOffset(layout, "aTexCoord", uvOffset);
            bool hasTangent = FindElementOffset(layout, "aTangent", tangentOffset);

            if (!hasPos)
                continue;

            glm::mat4 worldMatrix = modelSnap.worldMatrix;
            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(worldMatrix)));
            // Tangents are surface directions, not normals - transform with the plain model 3x3 (matching
            // lit.vert's `mat3(model) * aTangent`), not the inverse-transpose normal matrix above.
            glm::mat3 tangentMatrix = glm::mat3(worldMatrix);

            const std::vector<uint8_t>& vertexData = mesh->GetVertices();
            const std::vector<uint32_t>& indices = mesh->GetIndices();
            const std::vector<SubMesh>& submeshes = mesh->GetSubMeshes();

            for (size_t submeshIdx = 0; submeshIdx < submeshes.size(); submeshIdx++)
            {
                const SubMesh& submesh = submeshes[submeshIdx];
                uint32_t materialIndex = modelSnap.materialIndicesPerSubmesh[submeshIdx];

                size_t triangleCount = submesh.indexCount / 3;

                for (size_t t = 0; t < triangleCount; t++)
                {
                    GPUTrianglePos triPos{};
                    GPUTriangleAttrib triAttrib{};
                    triAttrib.uvMatID.w = (float)materialIndex;

                    for (int k = 0; k < 3; k++)
                    {
                        uint32_t vertexIdx = indices[submesh.indexOffset + t * 3 + k];
                        size_t vertexByteOffset = (size_t)vertexIdx * stride;

                        glm::vec3 localPos = ReadVec3(vertexData.data(), vertexByteOffset, posOffset);
                        glm::vec3 worldPos = glm::vec3(worldMatrix * glm::vec4(localPos, 1.0f));

                        glm::vec3 worldNormal(0.0f, 1.0f, 0.0f);
                        if (hasNormal)
                            worldNormal = glm::normalize(normalMatrix * ReadVec3(vertexData.data(), vertexByteOffset, normalOffset));

                        glm::vec2 uv(0.0f);
                        if (hasUV)
                            uv = ReadVec2(vertexData.data(), vertexByteOffset, uvOffset);

                        glm::vec3 worldTangent(1.0f, 0.0f, 0.0f);
                        if (hasTangent)
                            worldTangent = glm::normalize(tangentMatrix * ReadVec3(vertexData.data(), vertexByteOffset, tangentOffset));

                        if (k == 0)
                        {
                            triPos.v0 = glm::vec4(worldPos, 0.0f);
                            triAttrib.n0 = glm::vec4(worldNormal, uv.x);
                            triAttrib.uvMatID.x = uv.y;
                            triAttrib.t0 = glm::vec4(worldTangent, 0.0f);
                        }
                        else if (k == 1)
                        {
                            triPos.v1 = glm::vec4(worldPos, 0.0f);
                            triAttrib.n1 = glm::vec4(worldNormal, uv.x);
                            triAttrib.uvMatID.y = uv.y;
                            triAttrib.t1 = glm::vec4(worldTangent, 0.0f);
                        }
                        else
                        {
                            triPos.v2 = glm::vec4(worldPos, 0.0f);
                            triAttrib.n2 = glm::vec4(worldNormal, uv.x);
                            triAttrib.uvMatID.z = uv.y;
                            triAttrib.t2 = glm::vec4(worldTangent, 0.0f);
                        }
                    }

                    positions.push_back(triPos);
                    attribs.push_back(triAttrib);
                }
            }
        }

        if (positions.empty())
        {
            scene.bvhNodes = { BVHNode{ glm::vec3(0.0f), 0, glm::vec3(0.0f), 0 } };
            return scene;
        }

        std::vector<BVHPrimitive> primitives(positions.size());
        for (size_t i = 0; i < positions.size(); i++)
        {
            glm::vec3 v0 = glm::vec3(positions[i].v0);
            glm::vec3 v1 = glm::vec3(positions[i].v1);
            glm::vec3 v2 = glm::vec3(positions[i].v2);

            primitives[i].boundsMin = glm::min(v0, glm::min(v1, v2));
            primitives[i].boundsMax = glm::max(v0, glm::max(v1, v2));
            primitives[i].centroid = (v0 + v1 + v2) / 3.0f;
        }

        std::vector<uint32_t> order;
        scene.bvhNodes = BVHBuilder::Build(primitives, order);

        scene.trianglePositions.resize(order.size());
        scene.triangleAttribs.resize(order.size());
        for (size_t i = 0; i < order.size(); i++)
        {
            scene.trianglePositions[i] = positions[order[i]];
            scene.triangleAttribs[i] = attribs[order[i]];
        }

        return scene;
    }

}
