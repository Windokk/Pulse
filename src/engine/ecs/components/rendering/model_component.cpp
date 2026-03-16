#include "model_component.hpp"

#include "engine/core/engine.hpp"
#include "engine/core/resources/resources_manager.hpp"

#include "engine/rendering/renderer/renderer.hpp"
#include "engine/rendering/material/material.hpp"

#include "engine/ecs/objects/actors/actor.hpp"

#include "model_component.reflection.hpp"

#include "engine/rendering/lighting/shadow_manager.hpp"

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
        this->meshID = mesh->GetAssetID();
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
            this->meshID = mesh->GetAssetID();
        }        
    }

    void Model::UpdateReferenceInLevel()
    {
        if (!parent || !parent->level || !mesh || !activated)
        {
            return;
        }
        parent->level->meshes[parent->GetComponentIDInLevel(local_id)] = { parent->transform->GetTransformMatrix(), mesh.get() };
        
        Update();
    }

    void Model::SetMaterials(std::vector<std::shared_ptr<Rendering::Material>>&& newMaterials)
    {
        if(!activated)
            return;

        if (newMaterials.empty()) {
            DEBUG_ERROR("SetMaterials called with empty list");
        }

        this->materials = std::move(newMaterials);
        this->Update();
        materialsID.clear();
        for(int i = 0; i < materials.size(); i++){
            materialsID.push_back(materials[i]->GetAssetID());
        }
    }

    void Model::Update(){

        if(!activated)
            return;

        if (materials.size() > 0 && mesh != nullptr && parent->level && parent->level->IsLoaded()){

            std::shared_ptr<Transform> tr = parent->transform;

            std::vector<Rendering::DrawCommand> cmds = mesh->CreateDrawCommands(tr, parent->GetComponentIDInLevel(local_id), this->materials);

            std::vector<std::string> passesName;
            passesName.push_back("ForwardPass");
            passesName.push_back("EditorOutlineMaskPass");

            for(int i = 0; i < Core::GetEngine().GetRenderer()->GetShadowManager()->GetShadowMapsCount(); i++)
            {   
                passesName.push_back("ShadowPass"+std::to_string(i));    
            }

            Core::GetEngine().GetRenderer()->AddCommands(cmds, passesName);
        }
    }

    void Model::RemoveFromDrawList()
    {
        if (materials.size() > 0 && mesh != nullptr && parent->level->IsLoaded()){
            std::shared_ptr<Transform> tr = parent->transform;
            std::vector<uint32_t> cmdsID;
            for(int i = 0; i < mesh->GetSubMeshes().size(); i++){
                cmdsID.push_back(parent->GetComponentIDInLevel(local_id) + i);
            }

            
            std::vector<std::string> passesName;
            passesName.push_back("ForwardPass");

            for(int i = 0; i < Core::GetEngine().GetRenderer()->GetShadowManager()->GetShadowMapsCount(); i++)
            {   
                passesName.push_back("ShadowPass"+std::to_string(i));    
            }

            Core::GetEngine().GetRenderer()->RemoveCommands(cmdsID, passesName);
        }
    }

    void Model::Destroy()
    {
        RemoveFromDrawList();
    }

    std::shared_ptr<Component> Model::Clone() const
    {
        auto cloned = Object::Create<Model>(*this);

        return cloned;
    }

    void Model::OnFieldChanged(const FieldChangedEvent &event)
    {
        if(event.field->type == TypeID::Vector && event.field->container){
            Update();
            std::vector<std::shared_ptr<Rendering::Material>> mats = {};
            for(auto matID : materialsID){
                mats.push_back(Core::GetEngine().GetResourcesManager()->GetMaterial(Core::GetEngine().GetAssetIDManager()->GetAssetFromID(matID)->baseInfos.nameInProject));
            }
            SetMaterials(std::move(mats));
        }
        
        else if(event.field->type == TypeID::Asset){
            SetMesh(meshID);
        }
    }
}