#pragma once

#include <atomic>
#include <memory>
#include <vector>
#include <cstdint>
#include <future>
#include <random>

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

    class Renderer;
    class StorageBuffer;
    class Texture2D;
    class Cubemap;
    class ComputeShader;
    class ComputePipeline;

    // GPU-facing probe entry - mirrors the `Probe` struct in probe_trace.comp (std430 layout).
    struct GPUProbe
    {
        glm::vec4 position = glm::vec4(0.0f); // xyz = world-space position, w unused
    };

    // Max number of ProbeVolumes that can be simultaneously active. Kept as a small fixed cap (rather
    // than a truly dynamic count) so lit.frag can declare fixed-size sampler/uniform/SSBO arrays - see
    // MAX_PROBE_VOLUMES in lit.frag, which must match this value exactly. Capped at 2 (not higher) : each
    // extra volume costs 2 fragment-shader samplers (one irradiance + one distance atlas), and lit.frag
    // is already close to some drivers' hard "profile doesn't support more than 32 samplers" limit (seen
    // in practice on an NVIDIA driver, GL error C7612) once the existing albedo/IBL/shadow-map samplers
    // are counted - see the binding layout comment in lit.frag.
    constexpr int kMaxProbeVolumes = 2;

    // Upper bound on ProbeVolume::raysPerProbe (a 16x16 octahedral tile). MUST match MAX_TILE_TEXELS in
    // probe_irradiance_convolve.comp, which stages one whole tile in shared memory and therefore needs a
    // compile-time bound on it. Not a limitation in practice : past this point, raising the ray count is
    // the wrong lever anyway (cost is linear in it, noise only falls with its square root - the temporal
    // blend is where quality is cheap).
    constexpr int kMaxRaysPerProbe = 256;

    // Upper bound on ProbeVolume::probeUpdateStride (round-robin probe update, see the class comment).
    // Capped because the temporal hysteresis is raised to this power to keep the response time constant
    // (see kBaseTemporalHysteresis) : past 8 that exponent drives the per-update blend weight low enough
    // that each refresh is essentially a full overwrite, so the smoothing that makes a single noisy ray
    // fan usable stops working and the volume just flickers instead.
    constexpr int kMaxProbeUpdateStride = 8;

    // Base per-frame temporal hysteresis for the published atlas (see probe_temporal_blend.comp). An
    // exponential moving average has a fixed effective memory of ~1/(1-h) frames and a steady-state
    // noise floor of sqrt((1-h)/(1+h)) relative to one frame's raw noise, so this is a direct
    // response-time vs. grain trade : 0.97 settles in ~33 frames with a ~12% floor. It is deliberately
    // NOT pushed higher now that bounces are fed back across frames (see the class comment) - the bounce
    // chain is a cascade of these filters, so settling time multiplies by roughly the number of hops,
    // and a value like 0.99 that reads fine for a single-pass blend turns into seconds of visible lag
    // before indirect light finishes creeping into a room. UpdateVolume() raises it to the power of the
    // probe update stride so a volume's response in seconds is independent of how many frames it
    // spreads its probes over.
    constexpr float kBaseTemporalHysteresis = 0.97f;

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
    // Up to kMaxProbeVolumes ProbeVolumes can be simultaneously active - each gets its own full set of
    // GPU resources (grid, atlases) in a VolumeSlot below, traced/convolved/published independently every
    // frame. This is what lets a small, densely-packed local volume sit nested inside a much coarser
    // room-scale one : DDGI_PickVolume in lit.frag picks, per shading point, the SMALLEST active volume
    // whose grid actually contains that point, so the local volume automatically overrides the global one
    // wherever it applies, with no explicit priority field needed (see ProbeVolume's header comment for
    // the concrete case this exists for - an object too small for the room-scale grid to resolve at all).
    //
    // Multi-bounce is fed back ACROSS frames, not within one : each frame traces every updated probe
    // exactly once, and the bounce term those rays pick up at their hit points is read straight out of
    // the previous frame's published atlas (see probe_trace.comp). Frame N's atlas therefore already
    // carries every bounce frame N-1 had resolved, so bounce depth grows by one hop per frame and
    // converges to effectively unbounded depth - for the cost of a SINGLE trace pass per frame. The
    // previous design instead re-traced the whole ray set maxBounces times every frame for a hard cap of
    // maxBounces bounces, which was both several times more expensive and strictly less light (and, in
    // particular, visibly less colour bleed - the deep hops are where most of a Cornell box's red/green
    // wall tint on the white surfaces comes from).
    //
    // On top of that, a volume can spread its probes over several frames (ProbeVolume::probeUpdateStride,
    // RTXGI's round-robin probe update) : with a stride of N only the probes whose index is congruent to
    // the frame number mod N are traced, so the per-frame ray cost drops by N while each probe still
    // refreshes every N frames. Every stage of the update walks that same subset, and the temporal
    // hysteresis is raised to the Nth power so a volume's response time in SECONDS doesn't change with
    // the stride.
    //
    // Scope kept intentionally simple beyond that : a persistent BVH/triangle/material snapshot rebuilt
    // on demand rather than every frame (so moving static geometry doesn't affect the GI until
    // RebuildScene() is called again, shared across every volume since it's level-wide, not per-volume),
    // and no cross-fading at a volume's boundary (a fragment picks exactly one volume, no blend between
    // two overlapping ones). The ray fan IS given a fresh random rotation every frame per volume (see
    // m_RayRNG) specifically so the temporal hysteresis blend can average out angular aliasing over time
    // instead of locking in whatever a fixed ray set happened to sample once.
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
            // then - same as before a first RebuildScene() call). Shared across every active volume,
            // since the scene geometry itself doesn't depend on how many probe volumes exist.
            void RebuildScene(Levels::Level* level);

            // (Re)allocates `volume`'s probe grid SSBO and atlases to match its current bounds/
            // resolution. No-op if `volume` isn't currently active (see AddActiveVolume). Called
            // automatically by AddActiveVolume() and whenever an active volume's grid fields change.
            void RebuildGrid(Objects::Components::ProbeVolume* volume);

            // Activates `volume` if it isn't already active and there's room (see kMaxProbeVolumes) -
            // beyond the cap, activation is refused (logged) rather than silently evicting an existing
            // volume. A volume already active is a no-op (RebuildGrid(volume) is what re-syncs an
            // existing slot after a field edit, not this).
            void AddActiveVolume(Objects::Components::ProbeVolume* volume);

            // No-op unless `volume` is currently active. Frees that volume's GPU resources.
            void RemoveActiveVolume(Objects::Components::ProbeVolume* volume);

            // Dispatches this frame's probe ray-trace + border-fixup compute passes for every active
            // volume. No-op if there are no active volumes or no persistent scene has been built yet
            // (RebuildScene() not called). Called once per frame from Renderer::BeginFrame(), before
            // DrawFrame() executes the forward pass - so this frame's atlases are ready by the time
            // lit.frag samples them.
            void Update();

            bool HasActiveVolume() const { return !m_Volumes.empty(); }
            int GetActiveVolumeCount() const { return (int)m_Volumes.size(); }

            // ---- Async scene-build progress (drives the editor's "Baking GI probes" notification) ----
            // A RebuildScene() call flips IsSceneBuilding() true until Update() picks up the finished
            // background build; GetSceneBuildProgress() walks 0 -> 1 and GetSceneBuildPhase() returns a
            // static string literal ("Baking scene info" then "Building BVH"). All three are safe to poll
            // from the main thread every frame while the build runs on its worker thread.
            bool IsSceneBuilding() const { return m_SceneBuilding.load(std::memory_order_relaxed); }
            float GetSceneBuildProgress() const { return m_SceneBuildProgress.load(std::memory_order_relaxed); }
            const char* GetSceneBuildPhase() const { return m_SceneBuildPhase.load(std::memory_order_relaxed); }

            // True if at least one active volume has a fully-populated published atlas ready to sample -
            // gates ddgi_enabled in lit.frag. Per-index readiness (a freshly-activated volume needs a
            // frame to catch up) is IsVolumeReady() below.
            bool IsReady() const;

            // index is a slot index in [0, GetActiveVolumeCount()) - order is stable within a frame but
            // not guaranteed across Add/Remove calls, so callers (gl_api.cpp) re-enumerate every frame
            // rather than caching an index. IsVolumeReady() also requires that Update() has actually
            // dispatched a trace pass for that slot at least once - without it, a freshly (re)allocated
            // atlas whose trace/border-fixup shaders failed to compile would still report ready with
            // GPU-undefined contents, making lit.frag sample garbage/black instead of skipping it.
            bool IsVolumeReady(int index) const;
            std::shared_ptr<Texture2D> GetIrradianceAtlas(int index) const;
            std::shared_ptr<Texture2D> GetDistanceAtlas(int index) const;
            std::shared_ptr<StorageBuffer> GetProbeStateBuffer(int index) const;
            glm::vec3 GetGridOrigin(int index) const;
            glm::vec3 GetGridSpacing(int index) const;
            glm::ivec3 GetProbeCounts(int index) const;
            uint32_t GetTileSize(int index) const;
            uint32_t GetAtlasProbesPerRow(int index) const;
            uint32_t GetAtlasSize(int index) const;

        private:
            // Everything one active ProbeVolume needs on the GPU - see the class comment above for why
            // there can be more than one of these live at once.
            struct VolumeSlot
            {
                Objects::Components::ProbeVolume* volume = nullptr;

                std::shared_ptr<StorageBuffer> probeBuffer;
                uint32_t probeCount = 0;

                // One vec4 of per-probe state, bound at SSBO 14 for every probe compute pass and (per
                // volume) for the forward pass :
                //  - .w   : classification flag (0.0 = inactive, 1.0 = active), written by
                //           probe_classify.comp, read by DDGI_Diffuse in lit.frag and SampleIndirect in
                //           probe_trace.comp to exclude a probe from the blend entirely. Without it, a
                //           probe a uniform grid lands inside a protruding piece of geometry (wall
                //           relief, statue, pillar) sees mostly backfaces and bakes biased, self-lit
                //           radiance that leaks into whatever it's stuck inside via the trilinear blend.
                //  - .xyz : world-space relocation offset, written by probe_relocate.comp, added to the
                //           probe's grid position everywhere it's used (probe_trace.comp ray origins +
                //           SampleIndirect, lit.frag DDGI_Diffuse). RTXGI "Probe Relocation" : nudge a
                //           trapped/grazing probe back into open space (bounded to 0.45 * min grid
                //           spacing) so it stays useful instead of only being silenced by .w. Persistent
                //           across frames (integrated, then eased back toward zero once the probe has
                //           room). A relocated probe re-classifies active once its rays clear the geometry.
                // Both packed into one buffer so the forward pass needs only one SSBO binding per volume
                // (14, 15) rather than four. Initialized to (0,0,0,1) in RebuildGrid() - all-active,
                // zero offset - so nothing is wrongly excluded before the first classify/relocate
                // dispatch, and so re-running RebuildGrid() (grid resize, or ProbeVolume::enableRelocation
                // toggled) resets accumulated offsets.
                std::shared_ptr<StorageBuffer> probeStateBuffer;

                // Three atlases, same tile layout/size (tileSize = sqrt(raysPerProbe) texels + 1 texel of
                // border on each side, border-fixup pass keeps bilinear sampling from bleeding across
                // tiles), laid out as a square-ish grid of tiles :
                //  - rayAtlas : raw, single-sample-per-texel radiance written by probe_trace.comp (one
                //    ray per texel, no scatter/gather - see its header comment). Never sampled by
                //    lit.frag - scratch, rewritten every frame for whichever probes are updated.
                //  - irradianceAtlas : this frame's cosine-weighted irradiance map (see
                //    probe_irradiance_convolve.comp), raw and unsmoothed.
                //  - publishedAtlas : the atlas lit.frag samples AND probe_trace.comp reads back as the
                //    next frame's bounce source. probe_temporal_blend.comp exponentially blends
                //    irradianceAtlas into it every frame : a probe's tile is rebuilt from a single
                //    ray fan with a fresh random rotation, which is too noisy to shade with directly,
                //    and doubly so now that it also feeds itself back.
                // (There used to be a fourth, `bounceAtlas`, ping-ponged against irradianceAtlas so each
                // iteration of the in-frame bounce loop could read the previous one's result. Feeding
                // back from publishedAtlas across frames instead made both the loop and that extra
                // full-size RGBA16F target unnecessary.)
                std::shared_ptr<Texture2D> rayAtlas;
                std::shared_ptr<Texture2D> irradianceAtlas;
                std::shared_ptr<Texture2D> publishedAtlas;

                // Distance atlas trio, mirroring the irradiance one above texel-for-texel (same tile
                // layout/size, same border-fixup/temporal-blend treatment) but storing RG16F (mean hit
                // distance, mean hit distance^2) instead of RGBA16F radiance - see the header comment
                // above for what this feeds (the Chebyshev visibility test in lit.frag/probe_trace.comp).
                std::shared_ptr<Texture2D> rayDistAtlas;
                std::shared_ptr<Texture2D> distanceAtlas;
                std::shared_ptr<Texture2D> publishedDistanceAtlas;

                uint32_t tileSize = 0;
                uint32_t atlasProbesPerRow = 0;
                uint32_t atlasSize = 0;

                // Per-slot (not per-manager) : a volume added later than another shouldn't inherit an
                // unrelated frame count. Doubles as the round-robin phase (frameIndex % stride picks
                // which probes this frame updates) and as the "has this probe ever been published"
                // test that gives a tile a full overwrite on its first update instead of fading in
                // from black through the hysteresis blend.
                uint32_t frameIndex = 0;
            };

            void EnsureShaders();
            void UploadScene(const Raytracing::RaytraceScene& scene);

            VolumeSlot* FindSlot(Objects::Components::ProbeVolume* volume);
            const VolumeSlot* FindSlot(int index) const;
            void RebuildGrid(VolumeSlot& slot);
            void UpdateVolume(VolumeSlot& slot, Renderer* renderer);

            std::vector<VolumeSlot> m_Volumes;

            // ---- Persistent scene (rebuilt on demand, see RebuildScene()) - shared across every volume ----
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
            // Atomic : bumped on the main thread in RebuildScene(), read on the worker thread by the
            // in-flight build's progress callback so a superseded build stops reporting.
            std::atomic<uint64_t> m_SceneBuildGeneration{ 0 };
            uint64_t m_PendingSceneBuildGeneration = 0;
            std::vector<std::future<Raytracing::RaytraceScene>> m_AbandonedSceneBuilds;

            // Written by the background build's progress callback (worker thread), polled by the editor
            // via IsSceneBuilding()/GetSceneBuildProgress()/GetSceneBuildPhase() (main thread). The phase
            // pointer is always a static string literal, so storing/loading it as a bare pointer is safe.
            std::atomic<bool> m_SceneBuilding{ false };
            std::atomic<float> m_SceneBuildProgress{ 0.0f };
            std::atomic<const char*> m_SceneBuildPhase{ "" };

            // Snapshot of the live LightManager's lights, refreshed every frame in Update() (cheap - see
            // Raytracer::BuildScene() for the same pattern). Shared across every volume's trace dispatch.
            std::shared_ptr<StorageBuffer> m_LightBuffer;
            std::vector<LightData> m_FlatLights;

            // The loaded level's skybox, cosine-convolved - the exact same cubemap lit.frag samples as
            // ibl_irradianceMap. probe_trace.comp returns it for rays that escape the scene, instead of
            // the hardcoded blue-white constant it used to. Refreshed every frame in Update() rather
            // than cached at RebuildScene() time so swapping a level's skybox takes effect immediately
            // and an unloaded level can't leave a dangling reference. Null when the level has no
            // skybox, in which case sky radiance is zero - matching the forward pass, which gives a
            // skybox-less level no IBL either.
            std::shared_ptr<Cubemap> m_SkyIrradiance;

            std::shared_ptr<ComputeShader> m_TraceShader;
            std::shared_ptr<ComputePipeline> m_TracePipeline;
            std::shared_ptr<ComputeShader> m_ConvolveShader;
            std::shared_ptr<ComputePipeline> m_ConvolvePipeline;
            std::shared_ptr<ComputeShader> m_BorderFixupShader;
            std::shared_ptr<ComputePipeline> m_BorderFixupPipeline;
            std::shared_ptr<ComputeShader> m_ClassifyShader;
            std::shared_ptr<ComputePipeline> m_ClassifyPipeline;
            std::shared_ptr<ComputeShader> m_RelocateShader;
            std::shared_ptr<ComputePipeline> m_RelocatePipeline;
            std::shared_ptr<ComputeShader> m_TemporalBlendShader;
            std::shared_ptr<ComputePipeline> m_TemporalBlendPipeline;
            // Separate pipeline from m_TemporalBlendPipeline : that shader's uPublished image is
            // hardcoded to the rgba16f format qualifier the irradiance atlas uses, and GL requires an
            // image2D's declared format to match the bound texture's actual internal format - binding
            // an RG16F distance atlas to it would be invalid. probe_distance_temporal_blend.comp is
            // the same shader logic, just declared against rg16f instead.
            std::shared_ptr<ComputeShader> m_DistanceTemporalBlendShader;
            std::shared_ptr<ComputePipeline> m_DistanceTemporalBlendPipeline;

            // A fresh uniform-random rotation is applied to each volume's whole ray fan every frame
            // (same rotation for every bounce iteration within one volume's UpdateVolume() call, so the
            // multi-bounce feedback loop stays internally consistent - only the SET of directions traced
            // changes frame to frame, not within a frame). Without this, the fixed, deterministic ray
            // set means the exact same "lucky or unlucky" ray (e.g. one that happens to graze a small,
            // highly saturated surface like a tapestry, or a bright doorway) gets retraced every single
            // frame - the existing temporal hysteresis blend then just keeps confirming that one biased
            // sample instead of averaging it against others nearby, so it locks in as a stable aliasing
            // artifact (speckled color bleed, a stray bright patch) rather than converging toward the
            // true value the way real per-frame resampling would. Shared (not per-slot) since it's just
            // an RNG stream - every volume draws its own independent rotation from it each frame.
            std::mt19937 m_RayRNG;
    };
}
