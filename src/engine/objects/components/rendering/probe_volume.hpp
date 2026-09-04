#pragma once

#include <glm/glm.hpp>

#include "engine/objects/components/rendering/volume.hpp"

namespace Pulse::Engine::Objects::Components
{
    // Places a real-time diffuse GI probe grid in the level (see ProbeManager). Bounding box (halfExtent)
    // and its wireframe gizmo come from Volume - this adds the probe grid resolution/ray count and a
    // second gizmo (small white spheres, one per probe - see RebuildProbeVisualization()). V1 supports a
    // single active volume per level : activating a second one replaces the first (see
    // ProbeManager::SetActiveVolume). The grid is axis-aligned in world space, centered on this
    // component's actor position - it does not follow the actor's rotation/scale, only its translation.
    class CLASS() ProbeVolume : public Volume{
        public:
            ProbeVolume(std::shared_ptr<Actor> parent, uint32_t local_id);

            void Activate() override;
            void DeActivate() override;
            void Destroy() override;

            void Deserialize(const json componentData) override;
            ordered_json Serialize() override;

            std::shared_ptr<Component> Clone() const override;

            void OnFieldChanged(const FieldChangedEvent &event) override;

            // World-space position of probe (0,0,0) in the grid (the "min corner").
            glm::vec3 GetGridOrigin() const;

            // World-space distance between adjacent probes along each axis.
            glm::vec3 GetGridSpacing() const;

            FIELD(Editable)
            glm::ivec3 probeCounts = glm::ivec3(8, 4, 8);

            // Traced per probe per frame - must be a perfect square (8x8=64, 16x16=256, ...) since each
            // ray maps directly to one texel of the probe's octahedral irradiance tile (see
            // ProbeManager/probe_trace.comp). Values that aren't a perfect square are rounded down to the
            // nearest one when the grid is (re)built.
            FIELD(Editable)
            int raysPerProbe = 64;

            // Number of light bounces simulated synchronously every frame (1 = direct lighting seen by
            // probes only, same as no indirect at all; 2+ = each extra bounce re-traces all rays, feeding
            // back the previous bounce's freshly-convolved irradiance at each hit point - see
            // ProbeManager::Update()). Cost scales ~linearly with this value, so keep it low (2-4) for
            // real-time use.
            FIELD(Editable)
            int maxBounces = 2;

            DECLARE_DESCRIPTOR(ProbeVolume)

        protected:
            void RefreshDebugDrawCommands() override;

        private:
            // (Re)builds the probe-marker gizmo (small white spheres, one per probe position) - called
            // whenever halfExtent or probeCounts changes, since both affect probe layout.
            void RebuildProbeVisualization();

            Rendering::DebugShape* m_ProbeDebugShape = nullptr;
    };
}
