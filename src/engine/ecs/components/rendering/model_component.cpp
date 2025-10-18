#include "model_component.hpp"

#include "engine/rendering/renderer/renderer.hpp"

#include "engine/ecs/objects/actors/actor.hpp"

#include "engine/core/resources/resources_manager.hpp"

#include "engine/core/engine.hpp"

namespace Epoch::Engine::ECS::Components{
    

    Model::Model(Objects::Actor *parent, uint32_t local_id) : Component(parent, local_id)
    {
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

        if (materials.size() > 0 && mesh != nullptr && parent->level && parent->level->loaded){

            std::shared_ptr<Transform> tr = parent->transform;

            std::vector<Rendering::DrawCommand> cmds = mesh->CreateDrawCmds(tr, parent->GetComponentIDInScene(local_id), this->materials);

            Core::GetEngine().GetRenderer()->SubmitCommands(cmds, alreadySubmitted);

            alreadySubmitted = true;
        }
    }

    void Model::RemoveFromDrawList()
    {
        if (materials.size() > 0 && mesh != nullptr && parent->level->loaded){
            std::shared_ptr<Transform> tr = parent->transform;
            std::vector<Rendering::DrawCommand> cmds = mesh->CreateDrawCmds(tr, parent->GetComponentIDInScene(local_id), this->materials);
            Core::GetEngine().GetRenderer()->RemoveCommands(cmds);
        }
    }
}