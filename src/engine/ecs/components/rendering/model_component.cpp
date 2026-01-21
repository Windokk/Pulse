#include "model_component.hpp"

#include "engine/rendering/renderer/renderer.hpp"

#include "engine/ecs/objects/actors/actor.hpp"

#include "engine/core/resources/resources_manager.hpp"

#include "engine/core/engine.hpp"

#include "model_component.reflection.hpp"

namespace Pulse::Engine::ECS::Components{
    

    Model::Model(std::shared_ptr<Objects::Actor> parent, uint32_t localID) : Component(parent, localID)
    {
    }

    void Model::Deserialize(json componentData)
    {
        if (!parent)
            return;

        if (componentData.contains("mesh") && componentData["mesh"].is_string()) {
            const std::string& mesh_path = componentData["mesh"];
            SetMesh(mesh_path);
        }

        // Load materials directly from componentData
        const auto& localMaterials = componentData["materials"];
        int maxSlot = -1;
        for (auto it = localMaterials.begin(); it != localMaterials.end(); ++it) {
            int slot = std::stoi(it.key());
            if (slot > maxSlot) maxSlot = slot;
        }

        std::vector<std::shared_ptr<Rendering::Material>> materials(maxSlot + 1, nullptr);

        for (auto it = localMaterials.begin(); it != localMaterials.end(); ++it) {
            int slot = std::stoi(it.key());
            const std::string& materialPath = it.value();

            auto material = Core::GetEngine().GetResourcesManager()->GetMaterial(materialPath);
            if (!material) {
                DEBUG_ERROR("Failed to load material at path: " + materialPath + ". Using fallback.");
                material = Core::GetEngine().GetResourcesManager()->GetMaterial("materials/default.mat");

                if (!material) {
                    DEBUG_ERROR("Failed to load fallback material: materials/default.mat");
                }
            }

            materials[slot] = material;
        }

        SetMaterials(std::move(materials));

        if (componentData.contains("active") && componentData["active"].get<bool>())
            Activate();
        else
            DeActivate();
    }

    ordered_json Model::Serialize()
    {
        ordered_json comp;

        comp["type"] = "model";

        comp["active"] = activated;

        comp["mesh"] = Core::GetEngine().GetAssetIDManager()->GetAssetFromID(mesh->GetAssetID())->baseInfos.nameInProject;

        for(int i = 0; i < materials.size(); i++){
            std::string matNameInProject = Core::GetEngine().GetAssetIDManager()->GetAssetFromID(materials[i]->GetAssetID())->baseInfos.nameInProject;
            comp["materials"][std::to_string(i)] = matNameInProject;
        }

        return comp;
    }

    void Model::SetMesh(std::string meshPath)
    {
        if(!activated)
            return;
        
        this->mesh = Core::GetEngine().GetResourcesManager()->GetMesh(meshPath);
        this->Update();
        UpdateReferenceInLevel();
    }

    void Model::SetMesh(Filesystem::AssetID meshID)
    {
        if(!activated)
            return;

        std::shared_ptr<Filesystem::AssetInfos> asset = Core::GetEngine().GetAssetIDManager()->GetAssetFromID(meshID);
        
        if(asset){
            std::string name = asset->baseInfos.nameInProject;

            this->mesh = Core::GetEngine().GetResourcesManager()->GetMesh(name);
            this->Update();
            UpdateReferenceInLevel();
        }        
    }

    void Model::UpdateReferenceInLevel()
    {
        if (!parent || !parent->level || !mesh || !activated)
        {
            return;
        }
        parent->level->meshes[parent->GetComponentIDInScene(local_id)] = { parent->transform->GetTransformMatrix(), mesh.get() };
    }

    void Model::SetMaterials(std::vector<std::shared_ptr<Rendering::Material>>&& materials)
    {
        if(!activated)
            return;

        if (materials.empty()) {
            DEBUG_ERROR("SetMaterials called with empty list");
        }

        this->materials = std::move(materials);
        this->Update();
    }

    void Model::Update(){

        if(!activated)
            return;

        if (materials.size() > 0 && mesh != nullptr && parent->level && parent->level->IsLoaded()){

            std::shared_ptr<Transform> tr = parent->transform;

            std::vector<Rendering::DrawCommand> cmds = mesh->CreateDrawCmds(tr, parent->GetComponentIDInScene(local_id), this->materials);

            Core::GetEngine().GetRenderer()->SubmitCommands(cmds, alreadySubmitted);

            alreadySubmitted = true;
        }
    }

    void Model::RemoveFromDrawList()
    {
        if (materials.size() > 0 && mesh != nullptr && parent->level->IsLoaded()){
            std::shared_ptr<Transform> tr = parent->transform;
            std::vector<Rendering::DrawCommand> cmds = mesh->CreateDrawCmds(tr, parent->GetComponentIDInScene(local_id), this->materials);
            Core::GetEngine().GetRenderer()->RemoveCommands(cmds);
            alreadySubmitted = false;
        }
    }

    void Model::Destroy()
    {
        RemoveFromDrawList();
    }

    std::shared_ptr<Component> Model::Clone() const
    {
        auto cloned = std::make_shared<Model>(*this);

        return cloned;
    }
}

inline void WriteMeshAssetToModel(void* object, const void* value) {
    Pulse::Engine::ECS::Components::Model* comp = static_cast<Pulse::Engine::ECS::Components::Model*>(object);
    const Pulse::Engine::Filesystem::AssetID* meshID = static_cast<const Pulse::Engine::Filesystem::AssetID*>(value);
    comp->SetMesh(*meshID);
}

inline void ReadMeshAssetFromModel(void* object, void* outValue) {
    Pulse::Engine::ECS::Components::Model* comp = static_cast<Pulse::Engine::ECS::Components::Model*>(object);
    *static_cast<Pulse::Engine::Filesystem::AssetID*>(outValue) = comp->GetMesh()->GetAssetID();
}

inline void WriteMaterialAssetToModel(void* component, void* element, const void* editorValue)
{
    const Pulse::Engine::Filesystem::AssetID* matAsset = static_cast<const Pulse::Engine::Filesystem::AssetID*>(editorValue);

    auto assetInfo = Pulse::Engine::Core::GetEngine().GetAssetIDManager()->GetAssetFromID(*matAsset);
    if(!assetInfo) return;

    const std::string matNameInProj = assetInfo->baseInfos.nameInProject;
    
    auto* matPtr = static_cast<std::shared_ptr<Pulse::Engine::Rendering::Material>*>(element);

    std::shared_ptr<Pulse::Engine::Rendering::Material> newMat = Pulse::Engine::Core::GetEngine().GetResourcesManager()->GetMaterial(matNameInProj);

    if(newMat){
        *matPtr = std::move(newMat);
    }

    Pulse::Engine::ECS::Components::Model* comp = static_cast<Pulse::Engine::ECS::Components::Model*>(component);

    if(comp)
        comp->Update();
}

inline void ReadMaterialAssetFromModel(const void* element, void* outEditorValue)
{
    const std::shared_ptr<Pulse::Engine::Rendering::Material>* matPtr = static_cast<const std::shared_ptr<Pulse::Engine::Rendering::Material>*>(element);

    if (!matPtr || !(*matPtr))
    {
        *static_cast<std::string*>(outEditorValue) = "<none>";
        return;
    }

    *static_cast<Pulse::Engine::Filesystem::AssetID*>(outEditorValue) = (*matPtr)->GetAssetID();
}