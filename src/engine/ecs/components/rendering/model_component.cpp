#include "model_component.hpp"

#include "engine/rendering/renderer/renderer.hpp"

#include "engine/ecs/objects/actors/actor.hpp"

#include "engine/core/resources/resources_manager.hpp"

#include "engine/core/engine.hpp"

namespace Pulse::Engine::ECS::Components{
    

    Model::Model(std::shared_ptr<Objects::Actor> parent, uint32_t local_id) : Component(parent, local_id)
    {
    }

    void Model::Deserialize(json componentData, json levelData)
    {
        if(!parent)
            return;

        std::string mesh_name = componentData["mesh"];
        if (levelData["meshes"].contains(mesh_name) && levelData["meshes"][mesh_name].is_string()) {
            const std::string& mesh_path = levelData["meshes"][mesh_name];
            std::shared_ptr<Rendering::Mesh> mesh = Core::GetEngine().GetResourcesManager()->GetMesh(mesh_path);
            if(mesh)
                SetMesh(mesh);
            else
                DEBUG_ERROR("Failed to retrieve mesh : " + mesh_path);
        }

        const auto& localMaterials = componentData["materials"];
        const auto& globalMaterials = levelData["materials"];

        int maxSlot = -1;
        for (auto it = localMaterials.begin(); it != localMaterials.end(); ++it) {
            int slot = std::stoi(it.key());
            if (slot > maxSlot) maxSlot = slot;
        }

        std::vector<std::shared_ptr<Rendering::Material>> materials(maxSlot + 1, nullptr);

        for (auto it = localMaterials.begin(); it != localMaterials.end(); ++it) {
            int slot = std::stoi(it.key());
            const std::string& materialName = it.value();

            auto matIt = globalMaterials.find(materialName);
            std::string materialPath;

            if (matIt != globalMaterials.end()) {
                materialPath = matIt.value();
            } else {
                DEBUG_WARNING("Material '" + materialName
                        + "' not found in global materials. Using fallback.");
                materialPath = "materials\\default";
            }

            auto material = Core::GetEngine().GetResourcesManager()->GetMaterial(materialPath);
            if (!material) {
                DEBUG_ERROR("Failed to load material at path: " + materialPath
                        + ". Using fallback.");
                material = Core::GetEngine().GetResourcesManager()->GetMaterial("materials\\default");

                if (!material) {
                    DEBUG_ERROR("Failed to load fallback material: materials\\default");
                }
            }

            materials[slot] = material;
        }
        
        SetMaterials(std::move(materials));

        if(componentData.contains("active") && componentData["active"].get<bool>())
            Activate();
        else
            DeActivate();
    }

    json Model::Serialize()
    {
        json comp;

        comp["type"] = "model";

        comp["active"] = activated;

        std::string meshName = Core::GetEngine().GetAssetIDManager()->GetAssetFromID(mesh->GetAssetID())->baseInfos.name;

        comp["mesh"] = meshName;

        json ret;

        std::string meshNameInProject = Core::GetEngine().GetAssetIDManager()->GetAssetFromID(mesh->GetAssetID())->baseInfos.nameInProject;

        ret["meshes"][meshName] = meshNameInProject;

        for(int i = 0; i < materials.size(); i++){
            std::string matNameInProject = Core::GetEngine().GetAssetIDManager()->GetAssetFromID(materials[i]->GetAssetID())->baseInfos.nameInProject;
            std::string matName = Core::GetEngine().GetAssetIDManager()->GetAssetFromID(materials[i]->GetAssetID())->baseInfos.name;
            ret["materials"][matName] = matNameInProject;
            comp["materials"][std::to_string(i)] = matName;
        }

        ret["component"] = comp;

        return ret;
    }

    void Model::SetMesh(std::shared_ptr<Rendering::Mesh> mesh)
    {
        if(!activated)
            return;

        this->mesh = mesh;
        this->Update();
        UpdateReferenceInLevel();
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