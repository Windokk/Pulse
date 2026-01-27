#include "level.hpp"

#include <iostream>
#include <algorithm>

#include "engine/ecs/objects/objectID.hpp"
#include "engine/ecs/objects/actors/actor.hpp"
#include "engine/ecs/components/core/registry/component_registry.hpp"
#include "engine/core/engine.hpp"
#include "engine/core/resources/resources_manager.hpp"
#include "engine/ecs/objects/skybox/skybox.hpp"

namespace Pulse::Engine::Levels{


    Level::Level(std::string name, Filesystem::Path path)
    {
        this->name = name;
        this->path = path;
    }

    void DeserializeComponents(std::shared_ptr<ECS::Objects::Actor> a, json actorData){
        if(!actorData["components"].is_array()){
            DEBUG_ERROR("Couldn't deserialize actor's components as actor[components] is not an array");
            return; 
        }

        for(auto& component : actorData["components"]){
            
            if (!component.contains("type")) continue;
        
            const std::string& type = component["type"];

            if(type == "transform"){
                a->GetComponent<ECS::Components::Transform>()->Deserialize(component);
            }
            else if(type == "model"){
                std::shared_ptr<ECS::Components::Model> model = a->AddComponent<ECS::Components::Model>();
                model->Deserialize(component);
            }
            else if(type == "light"){
                std::shared_ptr<ECS::Components::Light> light = a->AddComponent<ECS::Components::Light>();
                light->Deserialize(component);
            }
            else if(type == "physics_body"){
                std::shared_ptr<ECS::Components::PhysicsBody> body = a->AddComponent<ECS::Components::PhysicsBody>();
                body->Deserialize(component);
            }
            else if(type == "camera"){
                std::shared_ptr<ECS::Components::Camera> cam = a->AddComponent<ECS::Components::Camera>();
                cam->Deserialize(component);
            }
            else if(type == "audio"){
                std::shared_ptr<ECS::Components::AudioSource> audio = a->AddComponent<ECS::Components::AudioSource>();
                audio->Deserialize(component);
            }
            else{
                //Custom component/Inherited component case
                //Note : The custom component has to be already registered
                ECS::Components::Component* rawComponent = Pulse::Engine::ECS::Components::GetComponentRegistry().CreateComponentByName(type);
                if (!rawComponent) {
                    DEBUG_WARNING("Unknown component type: " + type);
                    continue;
                }

                a->AddComponentRaw(rawComponent);
                rawComponent->Deserialize(component);
            }
        }
    }

    void DeserializeActor(std::shared_ptr<ECS::Objects::Actor> a, json data, json actor){

        if (actor.contains("children") && actor["children"].is_array() && !actor["children"].empty()) {
            for (auto& child : actor["children"]) {
                std::shared_ptr<ECS::Objects::Actor> b = ECS::Objects::Object::Create<ECS::Objects::Actor>(child["name"]);
                a->AddChild(b);
                DeserializeActor(b, data, child);
            }
        }

        DeserializeComponents(a, actor);
    }

    void Level::Deserialize(Filesystem::Path filePath)
    {
        std::string src = filePath.ReadFile();

        try {
            json data = json::parse(src);

            name = data["name"];

            for(auto& actor : data["actors"])
            {
                std::shared_ptr<ECS::Objects::Actor> a = ECS::Objects::Object::Create<ECS::Objects::Actor>(actor["name"]);
                AddActor(a);
                DeserializeActor(a, data, actor);
            }

            if(data.contains("skybox")){
                auto& skybox_folder = data["skybox"];
                if(skybox_folder.is_string()){
                    std::shared_ptr<Rendering::Shader> shader = Core::GetEngine().GetResourcesManager()->GetShader("shaders/cubemap/cubemap");
                    std::shared_ptr<Rendering::EnvironmentMap> envMap = Core::GetEngine().GetResourcesManager()->GetEnvMap(data["skybox"]);
                    
                    if(shader != nullptr && envMap != nullptr)
                    {
                        std::shared_ptr<ECS::Objects::Skybox> sb = ECS::Objects::Object::Create<ECS::Objects::Skybox>(envMap, shader);
                        this->skybox = sb;
                    }
                    else{
                        DEBUG_ERROR("Couldn't deserialize skybox : shader or cubemap missing");
                    }
                }
            }

        } catch (const json::parse_error& e) {
            DEBUG_ERROR("JSON parse error: " + (std::string)e.what());
            return;
        }
    }

    void SerializeActor(std::shared_ptr<Pulse::Engine::ECS::Objects::Actor> a, ordered_json* actorsArray){
        
        ordered_json actor;

        actor["name"] = a->GetName();

        for(auto& childrenID : a->GetChildrenID(false)){
            auto child = a->GetChild(childrenID);
            SerializeActor(std::dynamic_pointer_cast<ECS::Objects::Actor>(child), &actor["children"]);
        }

        for(auto& comp : a->GetComponents()){
            ordered_json serializedComp = comp->Serialize();
            actor["components"].push_back(serializedComp);
        }

        actorsArray->push_back(actor);
    }

    void Level::Serialize(Filesystem::Path filePath)
    {
        ordered_json actorsArray;

        ordered_json meshes;

        ordered_json materials;

        ordered_json full;

        full["name"] = name;

        std::vector<std::pair<ECS::ObjectID, std::shared_ptr<ECS::Objects::Actor>>> sortedActors(
            rootActors.begin(), rootActors.end()
        );

        std::sort(sortedActors.begin(), sortedActors.end(),
                [](const auto& a, const auto& b) { return a.first < b.first; });

        for(auto& [id, actorPtr] : sortedActors){
            SerializeActor(actorPtr, &actorsArray);
        }

        full["actors"] = actorsArray;

        if(skybox){
            full["skybox"] = Core::GetEngine().GetFileManager()->GetFileInfos(skybox->envMap->GetInfos()->filepath->full).nameInProject;
        }

        std::string fileContent = full.dump();

        filePath.WriteFile(fileContent);
    }

    void Level::SetBuildIndex(int buildIndex)
    {
        this->buildIndex = buildIndex;
    }

    void Level::RemoveActorRecursive(ECS::ObjectID actorID)
    {
        auto objPtr = Core::GetEngine().GetObjectIDManager()->GetObjectFromID(actorID);
        if (!objPtr)
            return;

        // Copy children IDs FIRST
        std::vector<ECS::ObjectID> children;
        children.reserve(objPtr->GetChildrenCount());

        for (int i = 0; i < objPtr->GetChildrenCount(); ++i)
            children.push_back(objPtr->GetChild(i)->GetID());

        // Now safely recurse
        for (ECS::ObjectID childID : children)
            RemoveActorRecursive(childID);

        // Finally remove this actor
        auto actorPtr = std::dynamic_pointer_cast<ECS::Objects::Actor>(objPtr);
        if (!actorPtr)
            return;

        actorPtr->Destroy();
    }

    void Level::Clear()
    {
        // Make a copy of keys because RemoveActorRecursive modifies rootActors
        std::vector<ECS::ObjectID> rootIDs;
        for (auto& [id, actorPtr] : rootActors)
            rootIDs.push_back(id);

        for (ECS::ObjectID id : rootIDs)
            RemoveActorRecursive(id);
    }

    void Level::OnLoad()
    {
        for(auto& [id,cam] : cameras){
            Core::GetEngine().GetCameraManager()->AddCamera(cam->parent->GetID(), cam);
        }

        for(int i = 0; i < lights.size(); i++){
            lights[i]->SetLightIndex(i);
        }

        for(auto& [id,model] : models){
            model->Update();
        }

        for(auto& [id,script] : scripts){
            script->OnLevelLoaded(); 
        }
    }

    void Level::Unload()
    {
        for(auto& [id,script] : scripts){
            script->OnLevelUnloaded();
        }
        
        Clear();

        loaded = false;
    }

    void Level::Tick()
    {
        for(auto& [id,script] : scripts){
            if(script->Active()){
                script->OnTick();
            }
        }
    }

    void Level::Play()
    {
        for(auto& [id,script] : scripts){
            if(script->Active()){
                script->OnPlay();
            }
        }
    }

    void Level::AddActor(std::shared_ptr<ECS::Objects::Actor> a)
    {
        rootActors.emplace(a->GetID(), a);
        a->SetLevel(this);
    }

    void Level::RemoveActor(ECS::ObjectID id)
    {
        if(rootActors.find(id) != rootActors.end())
            rootActors.erase(id);
    }

    std::shared_ptr<ECS::Objects::Actor> Level::GetActor(ECS::ObjectID id, bool recursive)
    {
        for (auto& [id, actorPtr] : rootActors)
        {
            if (actorPtr->GetID() == id)
                return actorPtr;

            if(recursive){
                std::vector<ECS::ObjectID> children = actorPtr->GetChildrenID(true);

                for(auto& _id : children){
                    std::shared_ptr<ECS::Objects::Actor> child = std::dynamic_pointer_cast<ECS::Objects::Actor>(Core::GetEngine().GetObjectIDManager()->GetObjectFromID(_id));
                    if(_id == id && child){
                        return child;
                    }
                }
            }
        }

        return nullptr;
    }

    std::vector<ECS::ObjectID> Level::GetActorsID(bool recursive)
    {
        std::vector<ECS::ObjectID> actorIDs;

        for (auto& [id, actorPtr] : rootActors)
        {
            actorIDs.push_back(actorPtr->GetID());

            if(recursive){
                std::vector<ECS::ObjectID> children = actorPtr->GetChildrenID(true);
                actorIDs.insert(actorIDs.end(), children.begin(), children.end());
            }
        }

        return actorIDs;
    }

    const std::string &Level::GetName() const
    {
        return name;
    }

    void Level::SetName(const std::string &name)
    {
        this->name = name;
    }

    void Level::RemoveComponent(const int idInLevel, const std::shared_ptr<ECS::Components::Component> compPtr)
    {
        compPtr->Destroy();
        if(compPtr->IsInstanceOf<ECS::Components::AudioSource>()){
            audioSources.erase(idInLevel);
        }
        else if(compPtr->IsInstanceOf<ECS::Components::Transform>()){
            transforms.erase(idInLevel);
        }
        else if(compPtr->IsInstanceOf<ECS::Components::PhysicsBody>()){
            physicsBodies.erase(idInLevel);
        }
        else if(compPtr->IsInstanceOf<ECS::Components::Camera>()){
            cameras.erase(idInLevel);
        }
        else if(compPtr->IsInstanceOf<ECS::Components::Script>()){
            scripts.erase(idInLevel);
        }
        else if(compPtr->IsInstanceOf<ECS::Components::Light>()){
            int index = lightComps.at(idInLevel)->GetLightIndex();
            std::rotate(lights.begin() + index, lights.begin() + index + 1, lights.end());
            lights.pop_back();
            lightComps.erase(idInLevel);
        }
        else if(compPtr->IsInstanceOf<ECS::Components::Model>()){
            meshes.erase(idInLevel);
        }
        else{
            DEBUG_ERROR("Tried removing a component of unknown type from actor : ", compPtr->parent->GetName(), " in level : ", GetName());
        }
    }
}