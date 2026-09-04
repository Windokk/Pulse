#include "probe_manager.hpp"

#include "engine/rendering/raytracing/bvh.hpp"
#include "engine/rendering/raytracing/raytrace_scene.hpp"

#include "engine/objects/components/rendering/probe_volume.hpp"

#include "engine/core/engine.hpp"

#include "engine/rendering/renderer/renderer.hpp"
#include "engine/rendering/renderer/renderer_api.hpp"
#include "engine/rendering/buffer/storage_buffer.hpp"
#include "engine/rendering/texture/texture.hpp"
#include "engine/rendering/shader/compute_shader.hpp"
#include "engine/rendering/pipeline/compute_pipeline.hpp"

#include "engine/debugging/logger.hpp"

#include <algorithm>
#include <cmath>
#include <chrono>

namespace Pulse::Engine::Rendering {

    ProbeManager::ProbeManager() = default;
    ProbeManager::~ProbeManager() = default;

    void ProbeManager::RebuildScene(Levels::Level* level)
    {
        if (!level)
            return;

        // CaptureSnapshot() is the only GL-touching (bindless texture handle resolution) part, and
        // it's cheap (bounded by model/material count) - do it here, synchronously, on the calling
        // (main/GL) thread. The expensive part (triangle flatten + BVH build, bounded by triangle
        // count) runs in the background via BuildFromSnapshot(), which touches no engine/GL state.
        Raytracing::RaytraceSceneSnapshot snapshot = Raytracing::SceneBuilder::CaptureSnapshot(level);

        // Don't let a still-running previous build block this call - std::async futures block their
        // destructor until the task finishes, so reassigning m_PendingSceneBuild directly would defeat
        // the point of going async. Park it instead; Update() drains finished entries opportunistically.
        if (m_PendingSceneBuild.valid())
            m_AbandonedSceneBuilds.push_back(std::move(m_PendingSceneBuild));

        m_SceneBuildGeneration++;
        m_PendingSceneBuildGeneration = m_SceneBuildGeneration;

        m_PendingSceneBuild = std::async(std::launch::async,
            [snapshot = std::move(snapshot)]() { return Raytracing::SceneBuilder::BuildFromSnapshot(snapshot); });

        m_SceneBuilt = false;
    }

    void ProbeManager::UploadScene(const Raytracing::RaytraceScene &scene)
    {
        if (scene.trianglePositions.empty())
        {
            m_SceneBuilt = false;
            return;
        }

        m_BVHBuffer = StorageBuffer::Create((uint32_t)(scene.bvhNodes.size() * sizeof(Raytracing::BVHNode)));
        m_BVHBuffer->SetData(scene.bvhNodes.data(), (uint32_t)(scene.bvhNodes.size() * sizeof(Raytracing::BVHNode)));

        m_PosBuffer = StorageBuffer::Create((uint32_t)(scene.trianglePositions.size() * sizeof(Raytracing::GPUTrianglePos)));
        m_PosBuffer->SetData(scene.trianglePositions.data(), (uint32_t)(scene.trianglePositions.size() * sizeof(Raytracing::GPUTrianglePos)));

        m_AttribBuffer = StorageBuffer::Create((uint32_t)(scene.triangleAttribs.size() * sizeof(Raytracing::GPUTriangleAttrib)));
        m_AttribBuffer->SetData(scene.triangleAttribs.data(), (uint32_t)(scene.triangleAttribs.size() * sizeof(Raytracing::GPUTriangleAttrib)));

        m_MatBuffer = StorageBuffer::Create((uint32_t)(scene.materials.size() * sizeof(Raytracing::GPUMaterial)));
        if (!scene.materials.empty())
            m_MatBuffer->SetData(scene.materials.data(), (uint32_t)(scene.materials.size() * sizeof(Raytracing::GPUMaterial)));

        m_SceneBuilt = true;
    }

    void ProbeManager::RebuildGrid()
    {
        if (!m_ActiveVolume)
            return;

        glm::ivec3 counts = glm::max(m_ActiveVolume->probeCounts, glm::ivec3(1));
        m_ProbeCount = (uint32_t)(counts.x * counts.y * counts.z);

        glm::vec3 origin = m_ActiveVolume->GetGridOrigin();
        glm::vec3 spacing = m_ActiveVolume->GetGridSpacing();

        std::vector<GPUProbe> probes(m_ProbeCount);
        uint32_t idx = 0;
        for (int z = 0; z < counts.z; z++)
            for (int y = 0; y < counts.y; y++)
                for (int x = 0; x < counts.x; x++)
                    probes[idx++].position = glm::vec4(origin + spacing * glm::vec3((float)x, (float)y, (float)z), 0.0f);

        m_ProbeBuffer = StorageBuffer::Create((uint32_t)(probes.size() * sizeof(GPUProbe)));
        m_ProbeBuffer->SetData(probes.data(), (uint32_t)(probes.size() * sizeof(GPUProbe)));

        // Each ray maps directly to one texel of the probe's octahedral tile (see probe_trace.comp), so
        // raysPerProbe is rounded down to the nearest perfect square here.
        int raysPerProbe = std::max(m_ActiveVolume->raysPerProbe, 1);
        m_TileSize = (uint32_t)std::max(1, (int)std::floor(std::sqrt((float)raysPerProbe)));

        m_AtlasProbesPerRow = std::max(1u, (uint32_t)std::ceil(std::sqrt((float)m_ProbeCount)));
        uint32_t strideWithBorder = m_TileSize + 2; // +1 texel of border on each side, see probe_border_fixup.comp
        m_AtlasSize = m_AtlasProbesPerRow * strideWithBorder;

        TextureSpecifications atlasSpec;
        atlasSpec.width = m_AtlasSize;
        atlasSpec.height = m_AtlasSize;
        atlasSpec.internalFormat = TextureInternalFormat::RGBA16F;
        atlasSpec.generateMips = false;
        atlasSpec.immutableStorage = true;
        atlasSpec.minFilter = TextureFilter::Linear;
        atlasSpec.magFilter = TextureFilter::Linear;
        atlasSpec.wrapS = TextureWrap::ClampEdge;
        atlasSpec.wrapT = TextureWrap::ClampEdge;
        // Same spec for all four - see the atlas comment in the header for why there are four.
        m_RayAtlas = Texture2D::Create(atlasSpec, nullptr);
        m_IrradianceAtlas = Texture2D::Create(atlasSpec, nullptr);
        m_BounceAtlas = Texture2D::Create(atlasSpec, nullptr);
        m_PublishedAtlas = Texture2D::Create(atlasSpec, nullptr);

        // Distance atlas quartet - same size/layout, RG16F (mean, mean^2) instead of RGBA16F radiance.
        TextureSpecifications distAtlasSpec = atlasSpec;
        distAtlasSpec.internalFormat = TextureInternalFormat::RG16F;

        m_RayDistAtlas = Texture2D::Create(distAtlasSpec, nullptr);
        m_DistanceAtlas = Texture2D::Create(distAtlasSpec, nullptr);
        m_BounceDistAtlas = Texture2D::Create(distAtlasSpec, nullptr);
        m_PublishedDistanceAtlas = Texture2D::Create(distAtlasSpec, nullptr);

        m_FrameIndex = 0;
    }

    void ProbeManager::SetActiveVolume(Objects::Components::ProbeVolume* volume)
    {
        m_ActiveVolume = volume;
        RebuildGrid();
    }

    void ProbeManager::ClearActiveVolume(Objects::Components::ProbeVolume* volume)
    {
        if (m_ActiveVolume != volume)
            return;

        m_ActiveVolume = nullptr;
        m_RayAtlas.reset();
        m_IrradianceAtlas.reset();
        m_BounceAtlas.reset();
        m_PublishedAtlas.reset();
        m_RayDistAtlas.reset();
        m_DistanceAtlas.reset();
        m_BounceDistAtlas.reset();
        m_PublishedDistanceAtlas.reset();
        m_ProbeBuffer.reset();
        m_ProbeCount = 0;
    }

    void ProbeManager::EnsureShaders()
    {
        if (m_TracePipeline && m_ConvolvePipeline && m_BorderFixupPipeline && m_TemporalBlendPipeline && m_DistanceTemporalBlendPipeline)
            return;

        Renderer* renderer = Core::GetEngine().GetRenderer();
        Filesystem::Path resRoot = Core::GetEngine().GetFileManager()->GetEngineResRoot();

        if (!m_TracePipeline)
        {
            m_TraceShader = ComputeShader::Create(resRoot / "shaders/compute/probe_trace.comp");
            if (!m_TraceShader)
            {
                DEBUG_ERROR("ProbeManager : failed to load probe_trace.comp");
                return;
            }

            ComputePipelineSpecifications specs;
            specs.shader = m_TraceShader;
            specs.debugName = "ProbeTrace";
            m_TracePipeline = renderer->GetOrAddComputePipeline(specs);
        }

        if (!m_ConvolvePipeline)
        {
            m_ConvolveShader = ComputeShader::Create(resRoot / "shaders/compute/probe_irradiance_convolve.comp");
            if (!m_ConvolveShader)
            {
                DEBUG_ERROR("ProbeManager : failed to load probe_irradiance_convolve.comp");
                return;
            }

            ComputePipelineSpecifications specs;
            specs.shader = m_ConvolveShader;
            specs.debugName = "ProbeIrradianceConvolve";
            m_ConvolvePipeline = renderer->GetOrAddComputePipeline(specs);
        }

        if (!m_BorderFixupPipeline)
        {
            m_BorderFixupShader = ComputeShader::Create(resRoot / "shaders/compute/probe_border_fixup.comp");
            if (!m_BorderFixupShader)
            {
                DEBUG_ERROR("ProbeManager : failed to load probe_border_fixup.comp");
                return;
            }

            ComputePipelineSpecifications specs;
            specs.shader = m_BorderFixupShader;
            specs.debugName = "ProbeBorderFixup";
            m_BorderFixupPipeline = renderer->GetOrAddComputePipeline(specs);
        }

        if (!m_TemporalBlendPipeline)
        {
            m_TemporalBlendShader = ComputeShader::Create(resRoot / "shaders/compute/probe_temporal_blend.comp");
            if (!m_TemporalBlendShader)
            {
                DEBUG_ERROR("ProbeManager : failed to load probe_temporal_blend.comp");
                return;
            }

            ComputePipelineSpecifications specs;
            specs.shader = m_TemporalBlendShader;
            specs.debugName = "ProbeTemporalBlend";
            m_TemporalBlendPipeline = renderer->GetOrAddComputePipeline(specs);
        }

        if (!m_DistanceTemporalBlendPipeline)
        {
            m_DistanceTemporalBlendShader = ComputeShader::Create(resRoot / "shaders/compute/probe_distance_temporal_blend.comp");
            if (!m_DistanceTemporalBlendShader)
            {
                DEBUG_ERROR("ProbeManager : failed to load probe_distance_temporal_blend.comp");
                return;
            }

            ComputePipelineSpecifications specs;
            specs.shader = m_DistanceTemporalBlendShader;
            specs.debugName = "ProbeDistanceTemporalBlend";
            m_DistanceTemporalBlendPipeline = renderer->GetOrAddComputePipeline(specs);
        }
    }

    void ProbeManager::Update()
    {
        // Drain any superseded builds that have since finished, so their futures (and the worker
        // threads behind them) don't pile up indefinitely - never blocks, only removes entries that
        // are already done.
        m_AbandonedSceneBuilds.erase(
            std::remove_if(m_AbandonedSceneBuilds.begin(), m_AbandonedSceneBuilds.end(),
                [](std::future<Raytracing::RaytraceScene>& f) {
                    return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
                }),
            m_AbandonedSceneBuilds.end());

        if (m_PendingSceneBuild.valid() &&
            m_PendingSceneBuild.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            Raytracing::RaytraceScene scene = m_PendingSceneBuild.get();

            // A newer RebuildScene() call may have superseded this one while it was building (see its
            // comment) - only the latest generation's result is worth uploading.
            if (m_PendingSceneBuildGeneration == m_SceneBuildGeneration)
                UploadScene(scene);
        }

        if (!m_SceneBuilt || !m_ActiveVolume || !m_RayAtlas || !m_IrradianceAtlas || !m_PublishedAtlas ||
            !m_RayDistAtlas || !m_DistanceAtlas || !m_PublishedDistanceAtlas || m_ProbeCount == 0)
            return;

        EnsureShaders();
        if (!m_TracePipeline || !m_ConvolvePipeline || !m_BorderFixupPipeline || !m_TemporalBlendPipeline || !m_DistanceTemporalBlendPipeline)
            return;

        Renderer* renderer = Core::GetEngine().GetRenderer();

        // Snapshot the live LightManager's lights every frame : cheap, and keeps this decoupled from the
        // live renderer buffer's lifecycle (same pattern as Raytracer::BuildScene()).
        const auto& sceneLights = renderer->GetLightManager()->GetLights();
        m_FlatLights.clear();
        m_FlatLights.reserve(sceneLights.size());
        for (auto& light : sceneLights)
            if (light)
                m_FlatLights.push_back(*light);

        if (!m_FlatLights.empty())
        {
            uint32_t neededSize = (uint32_t)(m_FlatLights.size() * sizeof(LightData));
            if (!m_LightBuffer || m_LightBuffer->GetSize() != neededSize)
                m_LightBuffer = StorageBuffer::Create(neededSize);
            m_LightBuffer->SetData(m_FlatLights.data(), neededSize);
        }

        // Ray atlas and irradiance atlas share the same tile size, so this doubles as both the ray count
        // per probe (trace) and the irradiance texel count per probe (convolve).
        uint32_t raysPerProbe = m_TileSize * m_TileSize;
        uint32_t totalRays = m_ProbeCount * raysPerProbe;
        uint32_t traceGroupsX = (totalRays + 63) / 64;
        uint32_t convolveGroupsX = traceGroupsX; // same texel count as raysPerProbe

        int maxBounces = std::max(m_ActiveVolume->maxBounces, 1);

        for (int bounce = 0; bounce < maxBounces; bounce++)
        {
            bool useIndirect = bounce > 0;

            // Trace : write raw per-ray radiance into the (never directly sampled) scratch ray atlas.
            m_TracePipeline->Bind();

            // SSBO binding points are global GL context state (glBindBufferBase), not scoped to this
            // pipeline - bindings 0-4 are what path_trace.comp AND lit.frag's LightBuffer use, and unlike
            // the offline raytracer (a one-off editor operation), ProbeManager::Update() runs every single
            // frame from Renderer::BeginFrame(), before the forward pass. Reusing binding 0 here silently
            // stole it away from lit.frag's `layout(std430, binding = 0) buffer LightBuffer` every frame
            // (LightManager only re-binds it when a light actually changes, not per-frame), which made
            // every light in the scene go dark as soon as a probe volume was active. Kept clear of 0-4 for
            // exactly that reason.
            m_BVHBuffer->Bind(8);
            m_PosBuffer->Bind(9);
            m_AttribBuffer->Bind(10);
            m_MatBuffer->Bind(11);
            if (m_LightBuffer)
                m_LightBuffer->Bind(12);
            m_ProbeBuffer->Bind(13);

            m_RayAtlas->BindImage(0, TextureAccess::ReadWrite);
            m_RayDistAtlas->BindImage(1, TextureAccess::ReadWrite);

            // Previous bounce iteration's freshly-convolved irradiance/distance - m_IrradianceAtlas and
            // m_DistanceAtlas always hold the latest convolved result by construction (see the ping-pong
            // swaps after the convolve dispatch below). Only bound when the shader will actually sample
            // them (bounce > 0) : these are raw texture-unit binds that bypass GLStateCache, so they're
            // skipped whenever not needed rather than left as a no-op cost every frame.
            // Unit numbers here (40/41) must match probe_trace.comp's uPrevIrradianceAtlas/
            // uPrevDistanceAtlas layout(binding=...) - see that shader's comment for why they're not 1/2/3.
            if (useIndirect)
            {
                m_IrradianceAtlas->Bind(40);
                m_DistanceAtlas->Bind(41);
            }

            m_TraceShader->SetInt("uProbeCount", (int)m_ProbeCount);
            m_TraceShader->SetInt("uTileSize", (int)m_TileSize);
            m_TraceShader->SetInt("uAtlasProbesPerRow", (int)m_AtlasProbesPerRow);
            m_TraceShader->SetInt("uAtlasSize", (int)m_AtlasSize);
            m_TraceShader->SetInt("uLightCount", (int)m_FlatLights.size());
            m_TraceShader->SetBool("uUseIndirect", useIndirect);
            m_TraceShader->SetVec3("uSkyColor", glm::vec3(0.05f, 0.07f, 0.1f));
            m_TraceShader->SetVec3("uGridOrigin", GetGridOrigin());
            m_TraceShader->SetVec3("uGridSpacing", GetGridSpacing());
            m_TraceShader->SetVec3("uProbeCounts", glm::vec3(GetProbeCounts()));

            // TextureFetch (not just ImageAccess) : the convolve pass below reads the ray atlas back via
            // a sampler2D (texelFetch), not imageLoad - GL_SHADER_IMAGE_ACCESS_BARRIER_BIT alone doesn't
            // order that.
            renderer->DispatchCompute(m_TracePipeline, traceGroupsX, 1, 1, MemoryBarrierBit::ImageAccess | MemoryBarrierBit::TextureFetch);

            // Convolve : turn the raw per-ray radiance into an actual cosine-weighted irradiance map -
            // see probe_irradiance_convolve.comp for why this step can't be skipped. Written into
            // m_BounceAtlas (the current "back buffer"), then swapped into m_IrradianceAtlas below.
            m_ConvolvePipeline->Bind();

            // Unit numbers here (42/43) must match probe_irradiance_convolve.comp's uRayAtlas/
            // uRayDistAtlas layout(binding=...) - see that shader's comment for why they're not 1/3.
            m_RayAtlas->Bind(42);
            m_BounceAtlas->BindImage(0, TextureAccess::WriteOnly);

            m_RayDistAtlas->Bind(43);
            m_BounceDistAtlas->BindImage(2, TextureAccess::WriteOnly);

            m_ConvolveShader->SetInt("uProbeCount", (int)m_ProbeCount);
            m_ConvolveShader->SetInt("uTileSize", (int)m_TileSize);
            m_ConvolveShader->SetInt("uAtlasProbesPerRow", (int)m_AtlasProbesPerRow);

            renderer->DispatchCompute(m_ConvolvePipeline, convolveGroupsX, 1, 1, MemoryBarrierBit::ImageAccess);

            // m_IrradianceAtlas/m_DistanceAtlas now point at this bounce's freshly-convolved result (and
            // m_BounceAtlas/m_BounceDistAtlas at the now-stale data from before this iteration, ready to
            // be overwritten as scratch next time) - see the ping-pong comment on these members in the
            // header.
            std::swap(m_IrradianceAtlas, m_BounceAtlas);
            std::swap(m_DistanceAtlas, m_BounceDistAtlas);
        }

        {
            // Border-fixup : duplicate tile-edge texels so bilinear sampling doesn't bleed across probes.
            // Runs once, after the bounce loop, directly on the final m_IrradianceAtlas.
            m_BorderFixupPipeline->Bind();

            m_IrradianceAtlas->BindImage(0, TextureAccess::ReadWrite);
            m_DistanceAtlas->BindImage(1, TextureAccess::ReadWrite);

            m_BorderFixupShader->SetInt("uProbeCount", (int)m_ProbeCount);
            m_BorderFixupShader->SetInt("uTileSize", (int)m_TileSize);
            m_BorderFixupShader->SetInt("uAtlasProbesPerRow", (int)m_AtlasProbesPerRow);

            // One thread per border texel : 4 * tileSize edge texels + 4 corner texels around each tile.
            uint32_t totalBorderTexels = m_ProbeCount * (m_TileSize * 4 + 4);
            uint32_t borderGroupsX = (totalBorderTexels + 63) / 64;
            // TextureFetch (not just ImageAccess) : this is the last writer before the forward pass
            // samples the atlas through `sampler2D ddgi_irradianceAtlas` in lit.frag -
            // GL_SHADER_IMAGE_ACCESS_BARRIER_BIT only orders subsequent imageLoad/imageStore, not sampler
            // reads, so without it the driver is free to let lit.frag see stale/incoherent atlas data
            // every frame (probe volumes silently doing nothing).
            renderer->DispatchCompute(m_BorderFixupPipeline, borderGroupsX, 1, 1, MemoryBarrierBit::ImageAccess | MemoryBarrierBit::TextureFetch);
        }

        {
            // Temporal blend : smooth this frame's raw, noisy N-bounce result into the persistent
            // published atlas lit.frag actually samples - see probe_temporal_blend.comp and the
            // m_PublishedAtlas comment in the header for why this can't just be skipped.
            m_TemporalBlendPipeline->Bind();

            // Unit number here (44) must match probe_temporal_blend.comp's uFresh layout(binding=...) -
            // see that shader's comment for why it's not 1.
            m_IrradianceAtlas->Bind(44);
            m_PublishedAtlas->BindImage(0, TextureAccess::ReadWrite);

            // Full overwrite on the first frame since a rebuild, when m_PublishedAtlas is still
            // uninitialized - same reasoning as probe_trace.comp's old per-frame uHysteresis. Shared
            // between the irradiance and distance blends below : nothing here calls for them to diverge.
            float hysteresis = m_FrameIndex == 0 ? 0.0f : 0.9f;

            m_TemporalBlendShader->SetInt("uAtlasSize", (int)m_AtlasSize);
            m_TemporalBlendShader->SetFloat("uHysteresis", hysteresis);

            uint32_t totalAtlasTexels = m_AtlasSize * m_AtlasSize;
            uint32_t blendGroupsX = (totalAtlasTexels + 63) / 64;
            renderer->DispatchCompute(m_TemporalBlendPipeline, blendGroupsX, 1, 1, MemoryBarrierBit::ImageAccess | MemoryBarrierBit::TextureFetch);

            // Distance atlas equivalent - separate pipeline (see m_DistanceTemporalBlendPipeline in the
            // header for why it can't reuse m_TemporalBlendPipeline), same texel count/group count since
            // both atlases share m_AtlasSize.
            m_DistanceTemporalBlendPipeline->Bind();

            // Unit number here (45) must match probe_distance_temporal_blend.comp's uFresh
            // layout(binding=...) - see that shader's comment for why it's not 1.
            m_DistanceAtlas->Bind(45);
            m_PublishedDistanceAtlas->BindImage(0, TextureAccess::ReadWrite);

            m_DistanceTemporalBlendShader->SetInt("uAtlasSize", (int)m_AtlasSize);
            m_DistanceTemporalBlendShader->SetFloat("uHysteresis", hysteresis);

            renderer->DispatchCompute(m_DistanceTemporalBlendPipeline, blendGroupsX, 1, 1, MemoryBarrierBit::ImageAccess | MemoryBarrierBit::TextureFetch);
        }

        m_FrameIndex++;
    }

    glm::vec3 ProbeManager::GetGridOrigin() const
    {
        return m_ActiveVolume ? m_ActiveVolume->GetGridOrigin() : glm::vec3(0.0f);
    }

    glm::vec3 ProbeManager::GetGridSpacing() const
    {
        return m_ActiveVolume ? m_ActiveVolume->GetGridSpacing() : glm::vec3(1.0f);
    }

    glm::ivec3 ProbeManager::GetProbeCounts() const
    {
        return m_ActiveVolume ? glm::max(m_ActiveVolume->probeCounts, glm::ivec3(1)) : glm::ivec3(0);
    }

}
