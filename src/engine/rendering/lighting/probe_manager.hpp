#pragma once

#include <memory>
#include <vector>
#include <cstdint>
#include <future>

#include <glm/glm.hpp>

#include "engine/rendering/lighting/light_manager.hpp"
#include "engine/rendering/raytracing/raytrace_scene.hpp"

namespace Pulse::Engine::Levels {
    class Level;
}

namespace Pulse::Engine::Objects::Components {
    class ProbeVolume;
}

namespace Pulse::Engine::Rendering {

    class StorageBuffer;
    class Texture2D;
    class ComputeShader;
    class ComputePipeline;

    // GPU-facing probe entry - mirrors the `Probe` struct in probe_trace.comp (std430 layout).
    struct GPUProbe
    {
        glm::vec4 position = glm::vec4(0.0f); // xyz = world-space position, w unused
    };

    // Owns the scene-wide resources for real-time diffuse GI via a grid of irradiance probes
    // (DDGI-like : a handful of rays traced per probe per frame against a persistent BVH, encoded into
    // an octahedral irradiance atlas, sampled in lit.frag). Mirrors LightManager/ShadowManager : a
    // manager owning GPU resources that span the whole scene, refreshed once per frame from
    // Renderer::BeginFrame().
    //
    // Each probe also stores a distance atlas (mean hit distance + mean hit distance^2 per octahedral
    // texel, alongside the irradiance one) so DDGI_Diffuse/SampleIndirect can weight down probes that
    // are occluded from the shading point with a Chebyshev visibility test - see DDGI_VisibilityWeight
    // in lit.frag. Without it, the trilinear probe blend has no notion of "is there a wall between this
    // probe and the point I'm shading", which is what makes classic irradiance probes leak light/shadow
    // through geometry (e.g. sun hitting a roof lighting the ceiling directly below it).
    //
    // V1 scope : a single active ProbeVolume per level (no streaming/blending between volumes), a
    // persistent BVH/triangle/material snapshot rebuilt on demand rather than every frame (so moving
    // static geometry doesn't affect the GI until RebuildScene() is called again), single-bounce direct
    // lighting per probe ray (no infinite-bounce feedback via re-sampling the probe grid at the hit
    // point), and no per-frame rotation of the ray set (some angular aliasing, but the temporal hysteresis
    // blend still smooths flicker from moving lights).
    class ProbeManager
    {
        public:
            ProbeManager();
            ~ProbeManager();

            // Kicks off a rebuild of the persistent BVH/triangle/material SSBOs from the level's
            // current geometry - NOT every frame, since rebuilding a full SAH BVH every frame would
            // defeat the point of a persistent one. Non-blocking: the (potentially expensive, for large
            // scenes) triangle-flatten + BVH-build work runs on a background thread via
            // SceneBuilder::CaptureSnapshot()+BuildFromSnapshot(); Update() picks up the result and
            // uploads it once ready (m_SceneBuilt stays false, and probe tracing is skipped, until
            // then - same as before a first RebuildScene() call).
            void RebuildScene(Levels::Level* level);

            // (Re)allocates the probe grid SSBO and irradiance atlas to match the active volume's
            // bounds/resolution. Called automatically by SetActiveVolume() and whenever the active
            // volume's grid fields change.
            void RebuildGrid();

            // Only one volume can be active at a time in this V1 - activating a new one replaces the
            // previous. ClearActiveVolume is a no-op unless `volume` is the currently active one (so a
            // volume being destroyed doesn't accidentally clear a different, newer active volume).
            void SetActiveVolume(Objects::Components::ProbeVolume* volume);
            void ClearActiveVolume(Objects::Components::ProbeVolume* volume);

            // Dispatches this frame's probe ray-trace + border-fixup compute passes. No-op if there's no
            // active volume or no persistent scene has been built yet (RebuildScene() not called). Called
            // once per frame from Renderer::BeginFrame(), before DrawFrame() executes the forward pass -
            // so this frame's atlas is ready by the time lit.frag samples it.
            void Update();

            bool HasActiveVolume() const { return m_ActiveVolume != nullptr; }

            // m_FrameIndex > 0 requires that Update() has actually dispatched a trace pass at least once
            // (gated behind EnsureShaders() succeeding) - without it, a freshly (re)allocated atlas whose
            // trace/border-fixup shaders failed to compile would still report ready with GPU-undefined
            // contents, making lit.frag sample garbage/black instead of falling back to flat ambient.
            bool IsReady() const { return m_SceneBuilt && m_ActiveVolume != nullptr && m_PublishedAtlas != nullptr && m_PublishedDistanceAtlas != nullptr && m_FrameIndex > 0; }

            std::shared_ptr<Texture2D> GetIrradianceAtlas() const { return m_PublishedAtlas; }
            std::shared_ptr<Texture2D> GetDistanceAtlas() const { return m_PublishedDistanceAtlas; }

            glm::vec3 GetGridOrigin() const;
            glm::vec3 GetGridSpacing() const;
            glm::ivec3 GetProbeCounts() const;

            uint32_t GetTileSize() const { return m_TileSize; }
            uint32_t GetAtlasProbesPerRow() const { return m_AtlasProbesPerRow; }
            uint32_t GetAtlasSize() const { return m_AtlasSize; }

        private:
            void EnsureShaders();
            void UploadScene(const Raytracing::RaytraceScene& scene);

            Objects::Components::ProbeVolume* m_ActiveVolume = nullptr;

            // ---- Persistent scene (rebuilt on demand, see RebuildScene()) ----
            bool m_SceneBuilt = false;
            std::shared_ptr<StorageBuffer> m_BVHBuffer;
            std::shared_ptr<StorageBuffer> m_PosBuffer;
            std::shared_ptr<StorageBuffer> m_AttribBuffer;
            std::shared_ptr<StorageBuffer> m_MatBuffer;

            // In-flight background BVH build kicked off by RebuildScene(), picked up by Update() once
            // ready. Guarded by a generation counter so a second RebuildScene() call (e.g. ProbeVolume
            // is activated twice while a level loads - once mid-Deserialize, once from Level::OnLoad())
            // can supersede a still-running first build without waiting on it: any future whose
            // generation no longer matches m_SceneBuildGeneration is drained and discarded rather than
            // uploaded. Superseded futures are parked in m_AbandonedSceneBuilds instead of being
            // reassigned directly over m_PendingSceneBuild, since destroying a still-running
            // std::async future blocks until it finishes - reassignment would defeat the whole point of
            // going async in the first place.
            std::future<Raytracing::RaytraceScene> m_PendingSceneBuild;
            uint64_t m_SceneBuildGeneration = 0;
            uint64_t m_PendingSceneBuildGeneration = 0;
            std::vector<std::future<Raytracing::RaytraceScene>> m_AbandonedSceneBuilds;

            // Snapshot of the live LightManager's lights, refreshed every frame in Update() (cheap - see
            // Raytracer::BuildScene() for the same pattern).
            std::shared_ptr<StorageBuffer> m_LightBuffer;
            std::vector<LightData> m_FlatLights;

            // ---- Probe grid (rebuilt on RebuildGrid(), see above) ----
            std::shared_ptr<StorageBuffer> m_ProbeBuffer;
            uint32_t m_ProbeCount = 0;

            // Four atlases, same tile layout/size (tileSize = sqrt(raysPerProbe) texels + 1 texel of
            // border on each side, border-fixup pass keeps bilinear sampling from bleeding across
            // tiles), laid out as a square-ish grid of tiles :
            //  - m_RayAtlas : raw, single-sample-per-texel radiance written by probe_trace.comp (one
            //    ray per texel, no scatter/gather - see its header comment). Never sampled directly by
            //    lit.frag - scratch, fully overwritten every bounce iteration.
            //  - m_IrradianceAtlas / m_BounceAtlas : ping-ponged cosine-weighted irradiance maps (see
            //    probe_irradiance_convolve.comp), ping-ponged across the maxBounces loop in Update() so
            //    each bounce iteration's trace pass reads the previous iteration's freshly-convolved
            //    result as its indirect term. By construction (the pair is swapped, not copied, after
            //    each iteration) m_IrradianceAtlas always ends the loop holding this frame's raw,
            //    unblended N-bounce result.
            //  - m_PublishedAtlas : the atlas lit.frag actually samples (via GetIrradianceAtlas()).
            //    probe_temporal_blend.comp exponentially blends m_IrradianceAtlas into this one every
            //    frame after the bounce loop - the whole N-bounce result is recomputed from scratch every
            //    frame with a fresh RNG seed (see probe_trace.comp), which is too noisy to sample
            //    directly whenever more than one light is in the scene (single-light NEE picks are
            //    effectively deterministic, so this only becomes visible - as flicker - with 2+ lights);
            //    this pass is what smooths that back out, the same way probe_trace.comp's own hysteresis
            //    blend did before the maxBounces rework made a single continuously-blended atlas
            //    ambiguous to feed back into itself bounce over bounce.
            std::shared_ptr<Texture2D> m_RayAtlas;
            std::shared_ptr<Texture2D> m_IrradianceAtlas;
            std::shared_ptr<Texture2D> m_BounceAtlas;
            std::shared_ptr<Texture2D> m_PublishedAtlas;

            // Distance atlas quartet, mirroring the irradiance one above texel-for-texel (same tile
            // layout/size, same ping-pong/border-fixup/temporal-blend treatment) but storing RG16F
            // (mean hit distance, mean hit distance^2) instead of RGBA16F radiance - see the header
            // comment above for what this feeds (the Chebyshev visibility test in lit.frag/
            // probe_trace.comp). Needs its own ping-pong pair (m_BounceDistAtlas) for the same reason
            // the irradiance atlas does : probe_trace.comp's indirect bounce term now also runs a
            // visibility test against the *previous* bounce iteration's distance atlas while the
            // current iteration writes fresh ray distances into m_RayDistAtlas.
            std::shared_ptr<Texture2D> m_RayDistAtlas;
            std::shared_ptr<Texture2D> m_DistanceAtlas;
            std::shared_ptr<Texture2D> m_BounceDistAtlas;
            std::shared_ptr<Texture2D> m_PublishedDistanceAtlas;

            uint32_t m_TileSize = 0;
            uint32_t m_AtlasProbesPerRow = 0;
            uint32_t m_AtlasSize = 0;

            std::shared_ptr<ComputeShader> m_TraceShader;
            std::shared_ptr<ComputePipeline> m_TracePipeline;
            std::shared_ptr<ComputeShader> m_ConvolveShader;
            std::shared_ptr<ComputePipeline> m_ConvolvePipeline;
            std::shared_ptr<ComputeShader> m_BorderFixupShader;
            std::shared_ptr<ComputePipeline> m_BorderFixupPipeline;
            std::shared_ptr<ComputeShader> m_TemporalBlendShader;
            std::shared_ptr<ComputePipeline> m_TemporalBlendPipeline;
            // Separate pipeline from m_TemporalBlendPipeline : that shader's uPublished image is
            // hardcoded to the rgba16f format qualifier the irradiance atlas uses, and GL requires an
            // image2D's declared format to match the bound texture's actual internal format - binding
            // an RG16F distance atlas to it would be invalid. probe_distance_temporal_blend.comp is
            // the same shader logic, just declared against rg16f instead.
            std::shared_ptr<ComputeShader> m_DistanceTemporalBlendShader;
            std::shared_ptr<ComputePipeline> m_DistanceTemporalBlendPipeline;

            uint32_t m_FrameIndex = 0;
    };
}
