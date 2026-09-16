#include "probe_volume.hpp"

#include "engine/rendering/renderer/renderer.hpp"
#include "engine/rendering/lighting/probe_manager.hpp"
#include "engine/rendering/mesh/mesh.hpp"
#include "engine/rendering/debug/debug_shapes.hpp"
#include "engine/objects/components/misc/transform.hpp"
#include "engine/objects/actors/actor.hpp"

#include "engine/core/engine.hpp"
#include "engine/filesystem/assetID.hpp"

#include "probe_volume.reflection.hpp"

namespace Pulse::Engine::Objects::Components{

    ProbeVolume::ProbeVolume(std::shared_ptr<Actor> parent, uint32_t local_id) : Volume(parent, local_id)
    {
        // A probe volume defaults to a room-sized box rather than Volume's generic 1x1x1 default.
        halfExtent = glm::vec3(5.0f, 3.0f, 5.0f);
    }

    glm::vec3 ProbeVolume::GetGridOrigin() const
    {
        glm::vec3 center = parent && parent->transform ? parent->transform->GetWorldPosition() : glm::vec3(0.0f);
        // Inset by half a cell (see GetGridSpacing()) so the outermost layer of probes sits inside the
        // volume's bounding box instead of exactly on its surface - a probe volume is typically sized
        // flush against a room's walls, and a probe placed right on/embedded in a wall sees it fill
        // almost its entire hemisphere, turning the octahedral tile's coarse angular resolution into a
        // visible faceted (diamond-shaped, from the octahedral projection) pattern directly on that wall.
        return center - halfExtent + GetGridSpacing() * 0.5f;
    }

    glm::vec3 ProbeVolume::GetGridSpacing() const
    {
        glm::ivec3 divisions = glm::max(probeCounts, glm::ivec3(1));
        return (halfExtent * 2.0f) / glm::vec3(divisions);
    }

    void ProbeVolume::RebuildProbeVisualization()
    {
        if (!activated || !parent)
            return;

        glm::ivec3 counts = glm::max(probeCounts, glm::ivec3(1));
        glm::vec3 spacing = GetGridSpacing();
        glm::vec3 localOrigin = -halfExtent + spacing * 0.5f; // relative to the actor's position - see GetGridOrigin()

        std::vector<glm::vec3> centers;
        centers.reserve((size_t)counts.x * (size_t)counts.y * (size_t)counts.z);
        for (int z = 0; z < counts.z; z++)
            for (int y = 0; y < counts.y; y++)
                for (int x = 0; x < counts.x; x++)
                    centers.push_back(localOrigin + spacing * glm::vec3((float)x, (float)y, (float)z));

        Filesystem::AssetID previousDebugMeshID;
        if (m_ProbeDebugShape && m_ProbeDebugShape->m_Mesh)
            previousDebugMeshID = m_ProbeDebugShape->m_Mesh->GetAssetID();

        delete m_ProbeDebugShape;
        m_ProbeDebugShape = new Rendering::DebugMultiSphere(centers, 0.08f, COL_RGBA(1.0f, 1.0f, 1.0f, 1.0f));

        if (previousDebugMeshID.GetAsInt() != 0)
            m_ProbeDebugShape->m_Mesh->SetAssetID(previousDebugMeshID);
        else
            m_ProbeDebugShape->m_Mesh->SetAssetID(GetEngineContext()->GetAssetIDManager()->GenerateNewID());

        RefreshDebugDrawCommands();
    }

    void ProbeVolume::RefreshDebugDrawCommands()
    {
        Volume::RefreshDebugDrawCommands();

        if (!m_ProbeDebugShape || !m_ProbeDebugShape->m_Mesh || !parent || !parent->level || !parent->level->IsLoaded())
            return;

        Rendering::DrawCommand cmd = {};

        cmd.boundsMax = m_ProbeDebugShape->m_Mesh->GetBoundsMax();
        cmd.boundsMin = m_ProbeDebugShape->m_Mesh->GetBoundsMin();
        cmd.indexCount = m_ProbeDebugShape->m_Mesh->GetIndexCount();
        cmd.indexOffset = 0;
        cmd.material = GetEngineContext()->GetRenderer()->GetDebugMaterial();
        cmd.mesh = m_ProbeDebugShape->m_Mesh;
        cmd.modelID = parent->GetComponentIDInLevel(local_id);
        cmd.modelMatrix = parent->transform->GetWorldMatrix();
        cmd.objectID = parent->GetID().GetAsInt();
        cmd.vertexCount = m_ProbeDebugShape->m_Mesh->GetVertexCount();

        // Its own pass (see Renderer::Init()), not "ForwardPass" like Volume's own box wireframe - lets
        // the editor hide just these markers (RenderPass::enabled) without touching ProbeVolume's
        // activation state, which must keep driving GI regardless of whether the markers are shown.
        GetEngineContext()->GetRenderer()->AddOrUpdateCommands({cmd}, {"ProbeGizmoPass"}, false);
    }

    void ProbeVolume::Activate()
    {
        RebuildProbeVisualization();
        Volume::Activate();

        if(parent && parent->level && parent->level->IsLoaded())
        {
            auto probeManager = GetEngineContext()->GetRenderer()->GetProbeManager();
            probeManager->RebuildScene(parent->level);
            probeManager->AddActiveVolume(this);
        }
    }

    void ProbeVolume::DeActivate()
    {
        Volume::DeActivate();

        if(parent && parent->level && parent->level->IsLoaded())
            GetEngineContext()->GetRenderer()->GetProbeManager()->RemoveActiveVolume(this);
    }

    void ProbeVolume::Destroy()
    {
        if(parent && parent->level && parent->level->IsLoaded())
            GetEngineContext()->GetRenderer()->GetProbeManager()->RemoveActiveVolume(this);

        if (m_ProbeDebugShape && m_ProbeDebugShape->m_Mesh && parent)
        {
            uint64_t cmdID = Rendering::MakeCommandID(m_ProbeDebugShape->m_Mesh->GetAssetID().GetAsInt(), parent->GetComponentIDInLevel(local_id), 0);
            GetEngineContext()->GetRenderer()->RemoveCommands({cmdID}, {"ProbeGizmoPass"}, false);
        }
        delete m_ProbeDebugShape;
        m_ProbeDebugShape = nullptr;

        Volume::Destroy();
    }

    void ProbeVolume::OnFieldChanged(const FieldChangedEvent &event)
    {
        std::string name = event.field->name;

        if (name == "halfExtent")
        {
            Volume::OnFieldChanged(event); // rebuilds the box wireframe
            RebuildProbeVisualization();   // halfExtent also changes probe spacing - regenerate the markers
        }
        else if (name == "probeCounts")
        {
            RebuildProbeVisualization();
        }

        // enableRelocation only needs the grid rebuilt so accumulated relocation offsets are zeroed
        // (RebuildGrid re-inits probeStateBuffer) - the per-frame dispatch is already gated on the flag.
        if (name == "halfExtent" || name == "probeCounts" || name == "raysPerProbe" || name == "enableRelocation")
        {
            if(parent && parent->level && parent->level->IsLoaded())
                GetEngineContext()->GetRenderer()->GetProbeManager()->RebuildGrid(this);
        }
    }

    void ProbeVolume::Deserialize(const json componentData)
    {
        auto getFloat = [&](const json& obj, const char* key, float fallback) -> float
        {
            if (!obj.contains(key) || !obj[key].is_number())
                return fallback;
            return obj[key].get<float>();
        };

        auto getInt = [&](const json& obj, const char* key, int fallback) -> int
        {
            if (!obj.contains(key) || !obj[key].is_number_integer())
                return fallback;
            return obj[key].get<int>();
        };

        if (componentData.contains("halfExtent") && componentData["halfExtent"].is_object())
        {
            const auto& e = componentData["halfExtent"];
            halfExtent = glm::vec3(getFloat(e, "x", 5.0f), getFloat(e, "y", 3.0f), getFloat(e, "z", 5.0f));
        }

        if (componentData.contains("probeCounts") && componentData["probeCounts"].is_object())
        {
            const auto& c = componentData["probeCounts"];
            probeCounts = glm::ivec3(getInt(c, "x", 8), getInt(c, "y", 4), getInt(c, "z", 8));
        }

        raysPerProbe = getInt(componentData, "raysPerProbe", 64);
        probeUpdateStride = getInt(componentData, "probeUpdateStride", 1);
        indirectIntensity = getFloat(componentData, "indirectIntensity", 1.0f);

        // "maxBounces" is deliberately not read any more : bounce depth stopped being a setting when
        // multi-bounce moved to a cross-frame feedback loop (see ProbeManager's class comment), and a
        // level authored before that change would otherwise keep asking for N times the ray cost to get
        // FEWER bounces than it now gets for free. Ignoring the old key silently is the right migration
        // - it simply stops being written on the next save.

        if (componentData.contains("enableRelocation") && componentData["enableRelocation"].is_boolean())
            enableRelocation = componentData["enableRelocation"].get<bool>();

        if (componentData.contains("active") && componentData["active"].is_boolean() && componentData["active"].get<bool>())
            Activate();
        else
            DeActivate();
    }

    ordered_json ProbeVolume::Serialize()
    {
        ordered_json comp;

        comp["type"] = "probeVolume";
        comp["active"] = activated;

        comp["halfExtent"]["x"] = halfExtent.x;
        comp["halfExtent"]["y"] = halfExtent.y;
        comp["halfExtent"]["z"] = halfExtent.z;

        comp["probeCounts"]["x"] = probeCounts.x;
        comp["probeCounts"]["y"] = probeCounts.y;
        comp["probeCounts"]["z"] = probeCounts.z;

        comp["raysPerProbe"] = raysPerProbe;
        comp["probeUpdateStride"] = probeUpdateStride;
        comp["indirectIntensity"] = indirectIntensity;
        comp["enableRelocation"] = enableRelocation;

        return comp;
    }

    std::shared_ptr<Component> ProbeVolume::Clone() const
    {
        return Object::Create<ProbeVolume>(*this);
    }

}
