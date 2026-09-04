#pragma once

#include <glm/glm.hpp>

#include "engine/objects/components/core/component.hpp"

#include "engine/core/attributes.hpp"

#include "engine/rendering/utils.hpp"

namespace Pulse::Engine::Rendering {
    class DebugShape;
}

namespace Pulse::Engine::Objects::Components
{
    // Abstract base for components that occupy a bounding-box region of the level (GI probe volumes,
    // and future volume types - post-process, reflection, trigger, ...). Owns the box wireframe gizmo
    // (mirrors PhysicsBody's DebugShape/draw-command lifecycle) so every volume type gets it for free;
    // subclasses only need to add whatever effect the volume actually drives. Not directly usable on its
    // own (no Clone()/GetDescriptor() override - GetDescriptor() stays pure virtual from Component), so
    // it never appears in the editor's "Add Component" list by itself.
    class CLASS() Volume : public Component{
        public:
            Volume(std::shared_ptr<Actor> parent, uint32_t local_id);

            void Activate() override;
            void DeActivate() override;
            void Destroy() override;

            void OnFieldChanged(const FieldChangedEvent &event) override;

            FIELD(Editable)
            glm::vec3 halfExtent = glm::vec3(1.0f);

            // Re-submits this volume's debug draw command(s) with the actor's current transform, without
            // regenerating any mesh - called whenever the actor moves (see
            // Transform::UpdateMeshReferencesInLevel, which needs this to be public). Subclasses that own
            // additional debug draw commands tied to the same transform (e.g. ProbeVolume's probe markers)
            // should override this to also refresh those, calling Volume::RefreshDebugDrawCommands() for
            // the box itself.
            virtual void RefreshDebugDrawCommands();

        protected:
            virtual COL_RGBA GetWireframeColor() const { return COL_RGBA(0.0f, 1.0f, 1.0f, 1.0f); }

            // (Re)builds the box wireframe mesh from `halfExtent` and refreshes its draw command. Called
            // on Activate() and whenever halfExtent changes - call again from a subclass whenever any
            // other field that affects its own additional debug visuals changes (see ProbeVolume).
            void RebuildDebugShape();

            void RemoveDebugShape();

            Rendering::DebugShape* m_DebugShape = nullptr;
    };
}
