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

    void Model::Deserialize(const json componentData)
    {
        if (!parent)
            return;

        auto getBool = [&](const char* key, bool fallback = false) -> bool
        {
            if (!componentData.contains(key) || !componentData[key].is_boolean())
                return fallback;
            return componentData[key].get<bool>();
        };

        auto getString = [&](const char* key) -> std::optional<std::string>
        {
            if (!componentData.contains(key) || !componentData[key].is_string())
                return std::nullopt;
            return componentData[key].get<std::string>();
        };

        // ---------------- MESH ----------------
        if (auto mesh = getString("mesh"))
        {
            SetMesh(*mesh);
        }

        // ---------------- MATERIALS ----------------
        std::vector<std::shared_ptr<Rendering::Material>> materials;

        if (!componentData.contains("materials") || !componentData["materials"].is_object())
        {
            DEBUG_ERROR("Model missing or invalid 'materials' block");
        }
        else
        {
            const auto& localMaterials = componentData["materials"];

            int maxSlot = -1;
            std::vector<std::pair<int, std::string>> parsed;

            // First pass: validate + collect safely
            for (auto it = localMaterials.begin(); it != localMaterials.end(); ++it)
            {
                int slot = 0;

                try
                {
                    slot = std::stoi(it.key());
                }
                catch (...)
                {
                    DEBUG_ERROR("Invalid material slot key (not integer): " + it.key());
                    continue;
                }

                if (slot < 0)
                {
                    DEBUG_ERROR("Negative material slot ignored: " + it.key());
                    continue;
                }

                if (!it.value().is_string())
                {
                    DEBUG_ERROR("Material path is not a string at slot: " + it.key());
                    continue;
                }

                std::string path = it.value().get<std::string>();

                parsed.emplace_back(slot, std::move(path));
                if (slot > maxSlot)
                    maxSlot = slot;
            }

            if (maxSlot >= 0)
                materials.resize(maxSlot + 1, nullptr);

            // Second pass: load materials
            for (auto& [slot, path] : parsed)
            {
                auto material =
                    GetEngineContext()->GetResourcesManager()->GetMaterial(path);

                if (!material)
                {
                    DEBUG_ERROR("Failed to load material: " + path + ", using fallback");

                    material =
                        GetEngineContext()->GetResourcesManager()->GetMaterial("materials/default.mat");

                    if (!material)
                    {
                        DEBUG_ERROR("Critical: fallback material missing (materials/default.mat)");
                        continue;
                    }
                }

                materials[slot] = material;
            }
        }

        SetMaterials(std::move(materials));

        // ---------------- ACTIVE ----------------
        if (getBool("active", false))
            Activate();
        else
            DeActivate();
    }

    ordered_json Model::Serialize()
    {
        ordered_json comp;

        comp["type"] = "model";

        comp["active"] = activated;

        comp["mesh"] = GetEngineContext()->GetAssetIDManager()->GetAssetFromID(mesh->GetAssetID())->baseInfos.nameInProject;

        for(int i = 0; i < materials.size(); i++){
            std::string matNameInProject = GetEngineContext()->GetAssetIDManager()->GetAssetFromID(materials[i]->GetAssetID())->baseInfos.nameInProject;
            comp["materials"][std::to_string(i)] = matNameInProject;
        }

        return comp;
    }

    void Model::SetMesh(std::string meshPath)
    {
        if(!activated)
            return;
        
        std::shared_ptr<Rendering::Mesh> newMesh = GetEngineContext()->GetResourcesManager()->GetMesh(meshPath);

        if(newMesh){
            if(mesh)
                RemoveFromDrawList();
            this->mesh = newMesh;
            UpdateReferenceInLevel();
            this->meshID = mesh->GetAssetID();
        }
        else{
            DEBUG_ERROR("Specified mesh path doesn't exist in project");
        }
    }

    void Model::SetMesh(Filesystem::AssetID meshID)
    {
        if(!activated)
            return;

        std::shared_ptr<Filesystem::AssetInfos> asset = GetEngineContext()->GetAssetIDManager()->GetAssetFromID(meshID);
        
        if(asset){
            std::string name = asset->baseInfos.nameInProject;

            if(mesh)
                RemoveFromDrawList();
            this->mesh = GetEngineContext()->GetResourcesManager()->GetMesh(name);
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
#ifdef  BUILD_EDITOR
            passesName.push_back("EditorOutlineMaskPass");
#endif

            GetEngineContext()->GetRenderer()->AddOrUpdateCommands(cmds, passesName, true);
        }
    }

    void Model::RemoveFromDrawList()
    {
        if (materials.size() > 0 && mesh != nullptr && parent->level->IsLoaded()){
            
            std::shared_ptr<Transform> tr = parent->transform;
            std::vector<uint64_t> cmdsID;
            for(int i = 0; i < mesh->GetSubMeshes().size(); i++){
                cmdsID.push_back(Rendering::MakeCommandID(mesh->GetAssetID().GetAsInt(), parent->GetComponentIDInLevel(local_id), i));
            }

            std::vector<std::string> passesName;
            passesName.push_back("ForwardPass");
#ifdef  BUILD_EDITOR
            passesName.push_back("EditorOutlineMaskPass");
#endif
            GetEngineContext()->GetRenderer()->RemoveCommands(cmdsID, passesName, true);
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
        //Materials
        if(event.field->type == TypeID::Vector && event.field->container){
            Update();
            std::vector<std::shared_ptr<Rendering::Material>> mats = {};
            for(auto matID : materialsID){
                mats.push_back(GetEngineContext()->GetResourcesManager()->GetMaterial(GetEngineContext()->GetAssetIDManager()->GetAssetFromID(matID)->baseInfos.nameInProject));
            }
            SetMaterials(std::move(mats));
        }
        
        //Mesh
        else if(event.field->type == TypeID::Asset){
            SetMesh(meshID);
        }
    }
}