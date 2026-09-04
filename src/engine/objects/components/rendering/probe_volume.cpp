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
        glm::vec3 center = parent && parent->transform ? parent->transform->GetPosition() : glm::vec3(0.0f);
        return center - halfExtent;
    }

    glm::vec3 ProbeVolume::GetGridSpacing() const
    {
        glm::ivec3 divisions = glm::max(probeCounts - glm::ivec3(1), glm::ivec3(1));
        return (halfExtent * 2.0f) / glm::vec3(divisions);
    }

    void ProbeVolume::RebuildProbeVisualization()
    {
        if (!activated || !parent)
            return;

        glm::ivec3 counts = glm::max(probeCounts, glm::ivec3(1));
        glm::vec3 spacing = GetGridSpacing();
        glm::vec3 localOrigin = -halfExtent; // relative to the actor's position - see GetGridOrigin()

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
        cmd.modelMatrix = parent->transform->GetTransformMatrix();
        cmd.objectID = parent->GetID().GetAsInt();
        cmd.vertexCount = m_ProbeDebugShape->m_Mesh->GetVertexCount();

        GetEngineContext()->GetRenderer()->AddOrUpdateCommands({cmd}, {"ForwardPass"}, false);
    }

    void ProbeVolume::Activate()
    {
        RebuildProbeVisualization();
        Volume::Activate();

        if(parent && parent->level && parent->level->IsLoaded())
        {
            auto probeManager = GetEngineContext()->GetRenderer()->GetProbeManager();
            probeManager->RebuildScene(parent->level);
            probeManager->SetActiveVolume(this);
        }
    }

    void ProbeVolume::DeActivate()
    {
        Volume::DeActivate();

        if(parent && parent->level && parent->level->IsLoaded())
            GetEngineContext()->GetRenderer()->GetProbeManager()->ClearActiveVolume(this);
    }

    void ProbeVolume::Destroy()
    {
        if(parent && parent->level && parent->level->IsLoaded())
            GetEngineContext()->GetRenderer()->GetProbeManager()->ClearActiveVolume(this);

        if (m_ProbeDebugShape && m_ProbeDebugShape->m_Mesh && parent)
        {
            uint64_t cmdID = Rendering::MakeCommandID(m_ProbeDebugShape->m_Mesh->GetAssetID().GetAsInt(), parent->GetComponentIDInLevel(local_id), 0);
            GetEngineContext()->GetRenderer()->RemoveCommands({cmdID}, {"ForwardPass"}, false);
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

        if (name == "halfExtent" || name == "probeCounts" || name == "raysPerProbe")
        {
            if(parent && parent->level && parent->level->IsLoaded())
                GetEngineContext()->GetRenderer()->GetProbeManager()->RebuildGrid();
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
        maxBounces = getInt(componentData, "maxBounces", 2);

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
        comp["maxBounces"] = maxBounces;

        return comp;
    }

    std::shared_ptr<Component> ProbeVolume::Clone() const
    {
        return Object::Create<ProbeVolume>(*this);
    }

}
