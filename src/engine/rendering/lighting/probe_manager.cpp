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
#include <random>

#include <glm/gtc/quaternion.hpp>

namespace Pulse::Engine::Rendering {

    namespace {

        // Small random rotation (random axis, uniformly-distributed on the sphere via Archimedes'
        // method, but a bounded rather than fully-random angle) - see the m_RayRNG comment in
        // probe_manager.hpp for why Update() applies a fresh one of these to each volume's ray fan every
        // frame. `maxAngleRadians` deliberately keeps this to a fraction of the angle between adjacent
        // rays in the tile (see the call site) : a FULL random rotation (the first version of this)
        // meant two consecutive frames could sample completely unrelated directions for the same nominal
        // texel, and right at an occlusion boundary that swings DDGI_VisibilityWeight's Chebyshev test
        // from ~1 (clear line of sight) to ~0 (blocked) every single frame - no amount of temporal
        // blending hides an input that keeps jumping between two extremes, only one that varies gently
        // around a stable value. A small jitter instead lets the temporal blend do what it's meant to :
        // antialias a stable signal, not average away wholesale resampling.
        glm::mat3 RandomRotation(std::mt19937& rng, float maxAngleRadians)
        {
            std::uniform_real_distribution<float> dist(0.0f, 1.0f);

            float z = dist(rng) * 2.0f - 1.0f;
            float theta = dist(rng) * 6.28318530718f;
            float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
            glm::vec3 axis(r * std::cos(theta), r * std::sin(theta), z);

            float angle = (dist(rng) * 2.0f - 1.0f) * maxAngleRadians;

            return glm::mat3_cast(glm::angleAxis(angle, axis));
        }

    }

    ProbeManager::ProbeManager() : m_RayRNG(std::random_device{}()) {}
    ProbeManager::~ProbeManager() = default;

    void ProbeManager::RebuildScene(Levels::Level* level)
    {
        if (!level)
            return;

        // CaptureSnapshot() is the only GL-touching (bindless texture handle resolution) part, and
        // it's cheap (bounded by model/material count) - do it here, synchronously, on the calling
        // (main/GL) thread. The expensive part (triangle flatten + BVH build, bounded by triangle
        // count) runs in the background via BuildFromSnapshot(), which touches no engine/GL state.
        Raytracing::RaytraceSceneSnapshot snapshot = Raytracing::SceneBuilder::CaptureSnapshot(level, /*excludeMasked=*/true);

        // Don't let a still-running previous build block this call - std::async futures block their
        // destructor until the task finishes, so reassigning m_PendingSceneBuild directly would defeat
        // the point of going async. Park it instead; Update() drains finished entries opportunistically.
        if (m_PendingSceneBuild.valid())
            m_AbandonedSceneBuilds.push_back(std::move(m_PendingSceneBuild));

        const uint64_t generation = m_SceneBuildGeneration.fetch_add(1, std::memory_order_relaxed) + 1;
        m_PendingSceneBuildGeneration = generation;

        // Reset the progress state the editor polls, before the worker thread starts touching it.
        m_SceneBuilding.store(true, std::memory_order_relaxed);
        m_SceneBuildProgress.store(0.0f, std::memory_order_relaxed);
        m_SceneBuildPhase.store("Baking scene info", std::memory_order_relaxed);

        m_PendingSceneBuild = std::async(std::launch::async,
            [this, generation, snapshot = std::move(snapshot)]()
            {
                auto onProgress = [this, generation](float fraction, const char* phase)
                {
                    // A newer RebuildScene() has superseded this build - stop writing progress so its
                    // stale percentage doesn't fight the current build's for the notification.
                    if (m_SceneBuildGeneration.load(std::memory_order_relaxed) != generation)
                        return;
                    m_SceneBuildProgress.store(fraction, std::memory_order_relaxed);
                    m_SceneBuildPhase.store(phase, std::memory_order_relaxed);
                };
                return Raytracing::SceneBuilder::BuildFromSnapshot(snapshot, onProgress);
            });

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

    ProbeManager::VolumeSlot* ProbeManager::FindSlot(Objects::Components::ProbeVolume* volume)
    {
        for (auto& slot : m_Volumes)
            if (slot.volume == volume)
                return &slot;
        return nullptr;
    }

    const ProbeManager::VolumeSlot* ProbeManager::FindSlot(int index) const
    {
        if (index < 0 || index >= (int)m_Volumes.size())
            return nullptr;
        return &m_Volumes[index];
    }

    void ProbeManager::RebuildGrid(VolumeSlot& slot)
    {
        Objects::Components::ProbeVolume* volume = slot.volume;
        if (!volume)
            return;

        glm::ivec3 counts = glm::max(volume->probeCounts, glm::ivec3(1));
        slot.probeCount = (uint32_t)(counts.x * counts.y * counts.z);

        glm::vec3 origin = volume->GetGridOrigin();
        glm::vec3 spacing = volume->GetGridSpacing();

        std::vector<GPUProbe> probes(slot.probeCount);
        uint32_t idx = 0;
        for (int z = 0; z < counts.z; z++)
            for (int y = 0; y < counts.y; y++)
                for (int x = 0; x < counts.x; x++)
                    probes[idx++].position = glm::vec4(origin + spacing * glm::vec3((float)x, (float)y, (float)z), 0.0f);

        slot.probeBuffer = StorageBuffer::Create((uint32_t)(probes.size() * sizeof(GPUProbe)));
        slot.probeBuffer->SetData(probes.data(), (uint32_t)(probes.size() * sizeof(GPUProbe)));

        // All-active until the first classify dispatch (see probe_classify.comp) actually runs - see the
        // probeActiveBuffer comment on VolumeSlot for why this can't start zeroed/uninitialized.
        std::vector<float> allActive(slot.probeCount, 1.0f);
        slot.probeActiveBuffer = StorageBuffer::Create((uint32_t)(allActive.size() * sizeof(float)));
        slot.probeActiveBuffer->SetData(allActive.data(), (uint32_t)(allActive.size() * sizeof(float)));

        // Each ray maps directly to one texel of the probe's octahedral tile (see probe_trace.comp), so
        // raysPerProbe is rounded down to the nearest perfect square here.
        int raysPerProbe = std::max(volume->raysPerProbe, 1);
        slot.tileSize = (uint32_t)std::max(1, (int)std::floor(std::sqrt((float)raysPerProbe)));

        slot.atlasProbesPerRow = std::max(1u, (uint32_t)std::ceil(std::sqrt((float)slot.probeCount)));
        uint32_t strideWithBorder = slot.tileSize + 2; // +1 texel of border on each side, see probe_border_fixup.comp
        slot.atlasSize = slot.atlasProbesPerRow * strideWithBorder;

        TextureSpecifications atlasSpec;
        atlasSpec.width = slot.atlasSize;
        atlasSpec.height = slot.atlasSize;
        atlasSpec.internalFormat = TextureInternalFormat::RGBA16F;
        atlasSpec.generateMips = false;
        atlasSpec.immutableStorage = true;
        atlasSpec.minFilter = TextureFilter::Linear;
        atlasSpec.magFilter = TextureFilter::Linear;
        atlasSpec.wrapS = TextureWrap::ClampEdge;
        atlasSpec.wrapT = TextureWrap::ClampEdge;
        // Same spec for all four - see the atlas comment on VolumeSlot for why there are four.
        slot.rayAtlas = Texture2D::Create(atlasSpec, nullptr);
        slot.irradianceAtlas = Texture2D::Create(atlasSpec, nullptr);
        slot.bounceAtlas = Texture2D::Create(atlasSpec, nullptr);
        slot.publishedAtlas = Texture2D::Create(atlasSpec, nullptr);

        // Distance atlas quartet - same size/layout, RG16F (mean, mean^2) instead of RGBA16F radiance.
        TextureSpecifications distAtlasSpec = atlasSpec;
        distAtlasSpec.internalFormat = TextureInternalFormat::RG16F;

        slot.rayDistAtlas = Texture2D::Create(distAtlasSpec, nullptr);
        slot.distanceAtlas = Texture2D::Create(distAtlasSpec, nullptr);
        slot.bounceDistAtlas = Texture2D::Create(distAtlasSpec, nullptr);
        slot.publishedDistanceAtlas = Texture2D::Create(distAtlasSpec, nullptr);

        slot.frameIndex = 0;
    }

    void ProbeManager::RebuildGrid(Objects::Components::ProbeVolume* volume)
    {
        VolumeSlot* slot = FindSlot(volume);
        if (!slot)
            return;

        RebuildGrid(*slot);
    }

    void ProbeManager::AddActiveVolume(Objects::Components::ProbeVolume* volume)
    {
        if (!volume || FindSlot(volume))
            return;

        if ((int)m_Volumes.size() >= kMaxProbeVolumes)
        {
            DEBUG_ERROR("ProbeManager : cannot activate ProbeVolume, already at the max of ", kMaxProbeVolumes, " simultaneously active volumes");
            return;
        }

        VolumeSlot slot;
        slot.volume = volume;
        m_Volumes.push_back(std::move(slot));
        RebuildGrid(m_Volumes.back());
    }

    void ProbeManager::RemoveActiveVolume(Objects::Components::ProbeVolume* volume)
    {
        auto it = std::find_if(m_Volumes.begin(), m_Volumes.end(),
            [volume](const VolumeSlot& slot) { return slot.volume == volume; });

        if (it != m_Volumes.end())
            m_Volumes.erase(it);
    }

    void ProbeManager::EnsureShaders()
    {
        if (m_TracePipeline && m_ConvolvePipeline && m_BorderFixupPipeline && m_ClassifyPipeline && m_TemporalBlendPipeline && m_DistanceTemporalBlendPipeline)
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

        if (!m_ClassifyPipeline)
        {
            m_ClassifyShader = ComputeShader::Create(resRoot / "shaders/compute/probe_classify.comp");
            if (!m_ClassifyShader)
            {
                DEBUG_ERROR("ProbeManager : failed to load probe_classify.comp");
                return;
            }

            ComputePipelineSpecifications specs;
            specs.shader = m_ClassifyShader;
            specs.debugName = "ProbeClassify";
            m_ClassifyPipeline = renderer->GetOrAddComputePipeline(specs);
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

    void ProbeManager::UpdateVolume(VolumeSlot& slot, Renderer* renderer)
    {
        if (!slot.volume || !slot.rayAtlas || !slot.irradianceAtlas || !slot.publishedAtlas ||
            !slot.rayDistAtlas || !slot.distanceAtlas || !slot.publishedDistanceAtlas || slot.probeCount == 0)
            return;

        // Ray atlas and irradiance atlas share the same tile size, so this doubles as both the ray count
        // per probe (trace) and the irradiance texel count per probe (convolve).
        uint32_t raysPerProbe = slot.tileSize * slot.tileSize;
        uint32_t totalRays = slot.probeCount * raysPerProbe;
        uint32_t traceGroupsX = (totalRays + 63) / 64;
        uint32_t convolveGroupsX = traceGroupsX; // same texel count as raysPerProbe

        // One rotation for the whole frame (all bounce iterations reuse it) - see the m_RayRNG comment
        // in the header for why this exists at all. Capped to a quarter of the average angular spacing
        // between adjacent rays (a tile spreads ~4*pi steradians over tileSize^2 samples, so that
        // spacing is ~2*sqrt(pi)/tileSize radians) so the jitter nudges each ray within its own
        // neighborhood instead of ever crossing into a completely unrelated part of the sphere - see
        // RandomRotation() for why that distinction matters.
        float raySpacingRadians = 3.5449077f / std::max(1.0f, (float)slot.tileSize);
        glm::mat3 rayRotation = RandomRotation(m_RayRNG, raySpacingRadians * 0.25f);

        int maxBounces = std::max(slot.volume->maxBounces, 1);
        glm::vec3 gridOrigin = slot.volume->GetGridOrigin();
        glm::vec3 gridSpacing = slot.volume->GetGridSpacing();
        glm::ivec3 probeCounts = glm::max(slot.volume->probeCounts, glm::ivec3(1));

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
            // exactly that reason. Reused across every volume's dispatch below (sequential, never bound
            // simultaneously for two different volumes) - only the forward pass's ProbeActive reads (see
            // gl_api.cpp) need one binding per volume alive at once.
            m_BVHBuffer->Bind(8);
            m_PosBuffer->Bind(9);
            m_AttribBuffer->Bind(10);
            m_MatBuffer->Bind(11);
            if (m_LightBuffer)
                m_LightBuffer->Bind(12);
            slot.probeBuffer->Bind(13);
            // Written by the classify dispatch below (bounce 0 only), read here by SampleIndirect on
            // every later bounce - see the probeActiveBuffer comment on VolumeSlot in the header.
            slot.probeActiveBuffer->Bind(14);

            slot.rayAtlas->BindImage(0, TextureAccess::ReadWrite);
            slot.rayDistAtlas->BindImage(1, TextureAccess::ReadWrite);

            // Previous bounce iteration's freshly-convolved irradiance/distance - slot.irradianceAtlas
            // and slot.distanceAtlas always hold the latest convolved result by construction (see the
            // ping-pong swaps after the convolve dispatch below). Only bound when the shader will
            // actually sample them (bounce > 0) : these are raw texture-unit binds that bypass
            // GLStateCache, so they're skipped whenever not needed rather than left as a no-op cost
            // every frame. Unit numbers here (40/41) must match probe_trace.comp's
            // uPrevIrradianceAtlas/uPrevDistanceAtlas layout(binding=...) - see that shader's comment
            // for why they're not 1/2/3.
            if (useIndirect)
            {
                slot.irradianceAtlas->Bind(40);
                slot.distanceAtlas->Bind(41);
            }

            m_TraceShader->SetInt("uProbeCount", (int)slot.probeCount);
            m_TraceShader->SetInt("uTileSize", (int)slot.tileSize);
            m_TraceShader->SetInt("uAtlasProbesPerRow", (int)slot.atlasProbesPerRow);
            m_TraceShader->SetInt("uAtlasSize", (int)slot.atlasSize);
            m_TraceShader->SetInt("uLightCount", (int)m_FlatLights.size());
            m_TraceShader->SetBool("uUseIndirect", useIndirect);
            m_TraceShader->SetVec3("uSkyColor", glm::vec3(0.8f, 0.9f, 1.0f));
            m_TraceShader->SetVec3("uGridOrigin", gridOrigin);
            m_TraceShader->SetVec3("uGridSpacing", gridSpacing);
            m_TraceShader->SetVec3("uProbeCounts", glm::vec3(probeCounts));
            m_TraceShader->SetMat3("uRayRotation", rayRotation);

            // TextureFetch (not just ImageAccess) : the convolve pass below reads the ray atlas back via
            // a sampler2D (texelFetch), not imageLoad - GL_SHADER_IMAGE_ACCESS_BARRIER_BIT alone doesn't
            // order that.
            renderer->DispatchCompute(m_TracePipeline, traceGroupsX, 1, 1, MemoryBarrierBit::ImageAccess | MemoryBarrierBit::TextureFetch);

            if (bounce == 0)
            {
                // Classify : one thread per probe, scans that probe's own tile of bounce 0's raw hits
                // (backface-hit ratio, see probe_classify.comp) and updates slot.probeActiveBuffer.
                // Bounce 0 never samples it (uUseIndirect is false there), so every later bounce
                // iteration this same frame - and the forward pass right after - sees this frame's own
                // classification, not a stale one. Only needs to run once : the hit geometry a ray finds
                // doesn't change between bounce iterations, only the shading at that hit point does.
                m_ClassifyPipeline->Bind();

                slot.rayAtlas->BindImage(0, TextureAccess::ReadOnly);

                m_ClassifyShader->SetInt("uProbeCount", (int)slot.probeCount);
                m_ClassifyShader->SetInt("uTileSize", (int)slot.tileSize);
                m_ClassifyShader->SetInt("uAtlasProbesPerRow", (int)slot.atlasProbesPerRow);

                // ShaderStorage (not ImageAccess) : this dispatch's output is an SSBO write
                // (slot.probeActiveBuffer), not an image store - read back both by later bounces' compute
                // dispatches in this same UpdateVolume() call and, further downstream, by lit.frag's
                // fragment shader once the forward pass runs.
                uint32_t classifyGroupsX = (slot.probeCount + 63) / 64;
                renderer->DispatchCompute(m_ClassifyPipeline, classifyGroupsX, 1, 1, MemoryBarrierBit::ShaderStorage);
            }

            // Convolve : turn the raw per-ray radiance into an actual cosine-weighted irradiance map -
            // see probe_irradiance_convolve.comp for why this step can't be skipped. Written into
            // slot.bounceAtlas (the current "back buffer"), then swapped into slot.irradianceAtlas below.
            m_ConvolvePipeline->Bind();

            // Unit numbers here (42/43) must match probe_irradiance_convolve.comp's uRayAtlas/
            // uRayDistAtlas layout(binding=...) - see that shader's comment for why they're not 1/3.
            slot.rayAtlas->Bind(42);
            slot.bounceAtlas->BindImage(0, TextureAccess::WriteOnly);

            slot.rayDistAtlas->Bind(43);
            slot.bounceDistAtlas->BindImage(2, TextureAccess::WriteOnly);

            m_ConvolveShader->SetInt("uProbeCount", (int)slot.probeCount);
            m_ConvolveShader->SetInt("uTileSize", (int)slot.tileSize);
            m_ConvolveShader->SetInt("uAtlasProbesPerRow", (int)slot.atlasProbesPerRow);

            renderer->DispatchCompute(m_ConvolvePipeline, convolveGroupsX, 1, 1, MemoryBarrierBit::ImageAccess);

            // slot.irradianceAtlas/slot.distanceAtlas now point at this bounce's freshly-convolved result
            // (and slot.bounceAtlas/slot.bounceDistAtlas at the now-stale data from before this
            // iteration, ready to be overwritten as scratch next time) - see the ping-pong comment on
            // these members in the header.
            std::swap(slot.irradianceAtlas, slot.bounceAtlas);
            std::swap(slot.distanceAtlas, slot.bounceDistAtlas);
        }

        {
            // Border-fixup : duplicate tile-edge texels so bilinear sampling doesn't bleed across probes.
            // Runs once, after the bounce loop, directly on the final slot.irradianceAtlas.
            m_BorderFixupPipeline->Bind();

            slot.irradianceAtlas->BindImage(0, TextureAccess::ReadWrite);
            slot.distanceAtlas->BindImage(1, TextureAccess::ReadWrite);

            m_BorderFixupShader->SetInt("uProbeCount", (int)slot.probeCount);
            m_BorderFixupShader->SetInt("uTileSize", (int)slot.tileSize);
            m_BorderFixupShader->SetInt("uAtlasProbesPerRow", (int)slot.atlasProbesPerRow);

            // One thread per border texel : 4 * tileSize edge texels + 4 corner texels around each tile.
            uint32_t totalBorderTexels = slot.probeCount * (slot.tileSize * 4 + 4);
            uint32_t borderGroupsX = (totalBorderTexels + 63) / 64;
            // TextureFetch (not just ImageAccess) : this is the last writer before the forward pass
            // samples the atlas through `sampler2D ddgi_irradianceAtlas[]` in lit.frag -
            // GL_SHADER_IMAGE_ACCESS_BARRIER_BIT only orders subsequent imageLoad/imageStore, not sampler
            // reads, so without it the driver is free to let lit.frag see stale/incoherent atlas data
            // every frame (probe volumes silently doing nothing).
            renderer->DispatchCompute(m_BorderFixupPipeline, borderGroupsX, 1, 1, MemoryBarrierBit::ImageAccess | MemoryBarrierBit::TextureFetch);
        }

        {
            // Temporal blend : smooth this frame's raw, noisy N-bounce result into the persistent
            // published atlas lit.frag actually samples - see probe_temporal_blend.comp and the
            // publishedAtlas comment on VolumeSlot for why this can't just be skipped.
            m_TemporalBlendPipeline->Bind();

            // Unit number here (44) must match probe_temporal_blend.comp's uFresh layout(binding=...) -
            // see that shader's comment for why it's not 1.
            slot.irradianceAtlas->Bind(44);
            slot.publishedAtlas->BindImage(0, TextureAccess::ReadWrite);

            // Full overwrite on the first frame since this slot was (re)built, when publishedAtlas is
            // still uninitialized - same reasoning as probe_trace.comp's old per-frame uHysteresis.
            // Shared between the irradiance and distance blends below : nothing here calls for them to
            // diverge.
            //
            // 0.99, not the 0.97 this used to be : an exponential moving average doesn't converge to zero
            // noise no matter how long it runs - it has a fixed effective memory of ~1/(1-hysteresis)
            // frames, and its steady-state noise floor (relative to a single frame's raw noise) is
            // sqrt((1-hysteresis)/(1+hysteresis)). At 0.97 that floor is ~12% - a real, persistent grainy
            // noise on flat, brightly-lit surfaces (e.g. a plain wall) that waiting longer never removes,
            // confirmed by A/B testing the exact same spot after 20s of runtime with no change. At 0.99
            // the floor drops to ~7% (roughly 1.7x quieter) for zero extra GPU cost - unlike raising
            // raysPerProbe, whose cost scales linearly while noise only falls with its square root. The
            // trade is slower response to an actual lighting change (~3x the settling time of 0.97), which
            // reads fine for a mostly-static architectural scene explored in real time.
            float hysteresis = slot.frameIndex == 0 ? 0.0f : 0.99f;

            m_TemporalBlendShader->SetInt("uAtlasSize", (int)slot.atlasSize);
            m_TemporalBlendShader->SetFloat("uHysteresis", hysteresis);

            uint32_t totalAtlasTexels = slot.atlasSize * slot.atlasSize;
            uint32_t blendGroupsX = (totalAtlasTexels + 63) / 64;
            renderer->DispatchCompute(m_TemporalBlendPipeline, blendGroupsX, 1, 1, MemoryBarrierBit::ImageAccess | MemoryBarrierBit::TextureFetch);

            // Distance atlas equivalent - separate pipeline (see m_DistanceTemporalBlendPipeline in the
            // header for why it can't reuse m_TemporalBlendPipeline), same texel count/group count since
            // both atlases share slot.atlasSize.
            m_DistanceTemporalBlendPipeline->Bind();

            // Unit number here (45) must match probe_distance_temporal_blend.comp's uFresh
            // layout(binding=...) - see that shader's comment for why it's not 1.
            slot.distanceAtlas->Bind(45);
            slot.publishedDistanceAtlas->BindImage(0, TextureAccess::ReadWrite);

            m_DistanceTemporalBlendShader->SetInt("uAtlasSize", (int)slot.atlasSize);
            m_DistanceTemporalBlendShader->SetFloat("uHysteresis", hysteresis);

            renderer->DispatchCompute(m_DistanceTemporalBlendPipeline, blendGroupsX, 1, 1, MemoryBarrierBit::ImageAccess | MemoryBarrierBit::TextureFetch);
        }

        slot.frameIndex++;
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
            if (m_PendingSceneBuildGeneration == m_SceneBuildGeneration.load(std::memory_order_relaxed))
                UploadScene(scene);

            // The pending build always holds the newest generation (RebuildScene() parks the previous
            // one in m_AbandonedSceneBuilds rather than overwriting it here), so its completion is the
            // end of the build the editor is showing progress for.
            m_SceneBuildProgress.store(1.0f, std::memory_order_relaxed);
            m_SceneBuilding.store(false, std::memory_order_relaxed);
        }

        if (!m_SceneBuilt || m_Volumes.empty())
            return;

        EnsureShaders();
        if (!m_TracePipeline || !m_ConvolvePipeline || !m_BorderFixupPipeline || !m_ClassifyPipeline || !m_TemporalBlendPipeline || !m_DistanceTemporalBlendPipeline)
            return;

        Renderer* renderer = Core::GetEngine().GetRenderer();

        // Snapshot the live LightManager's lights every frame : cheap, and keeps this decoupled from the
        // live renderer buffer's lifecycle (same pattern as Raytracer::BuildScene()). Shared across every
        // volume's trace dispatch below.
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

        for (auto& slot : m_Volumes)
            UpdateVolume(slot, renderer);
    }

    bool ProbeManager::IsReady() const
    {
        if (!m_SceneBuilt)
            return false;

        for (auto& slot : m_Volumes)
            if (slot.publishedAtlas && slot.publishedDistanceAtlas && slot.frameIndex > 0)
                return true;

        return false;
    }

    bool ProbeManager::IsVolumeReady(int index) const
    {
        const VolumeSlot* slot = FindSlot(index);
        return m_SceneBuilt && slot && slot->publishedAtlas && slot->publishedDistanceAtlas && slot->frameIndex > 0;
    }

    std::shared_ptr<Texture2D> ProbeManager::GetIrradianceAtlas(int index) const
    {
        const VolumeSlot* slot = FindSlot(index);
        return slot ? slot->publishedAtlas : nullptr;
    }

    std::shared_ptr<Texture2D> ProbeManager::GetDistanceAtlas(int index) const
    {
        const VolumeSlot* slot = FindSlot(index);
        return slot ? slot->publishedDistanceAtlas : nullptr;
    }

    std::shared_ptr<StorageBuffer> ProbeManager::GetProbeActiveBuffer(int index) const
    {
        const VolumeSlot* slot = FindSlot(index);
        return slot ? slot->probeActiveBuffer : nullptr;
    }

    glm::vec3 ProbeManager::GetGridOrigin(int index) const
    {
        const VolumeSlot* slot = FindSlot(index);
        return (slot && slot->volume) ? slot->volume->GetGridOrigin() : glm::vec3(0.0f);
    }

    glm::vec3 ProbeManager::GetGridSpacing(int index) const
    {
        const VolumeSlot* slot = FindSlot(index);
        return (slot && slot->volume) ? slot->volume->GetGridSpacing() : glm::vec3(1.0f);
    }

    glm::ivec3 ProbeManager::GetProbeCounts(int index) const
    {
        const VolumeSlot* slot = FindSlot(index);
        return (slot && slot->volume) ? glm::max(slot->volume->probeCounts, glm::ivec3(1)) : glm::ivec3(0);
    }

    uint32_t ProbeManager::GetTileSize(int index) const
    {
        const VolumeSlot* slot = FindSlot(index);
        return slot ? slot->tileSize : 0;
    }

    uint32_t ProbeManager::GetAtlasProbesPerRow(int index) const
    {
        const VolumeSlot* slot = FindSlot(index);
        return slot ? slot->atlasProbesPerRow : 0;
    }

    uint32_t ProbeManager::GetAtlasSize(int index) const
    {
        const VolumeSlot* slot = FindSlot(index);
        return slot ? slot->atlasSize : 0;
    }

}
