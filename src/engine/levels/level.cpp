#include "level.hpp"
#include <iostream>

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

    void DeserializeComponents(std::shared_ptr<ECS::Objects::Actor> a, json levelData, json actorData){
        if(!actorData["components"].is_array()){
            DEBUG_ERROR("Couldn't deserialize actor's components as actor[components] is not an array");
            return; 
        }

        for(auto& component : actorData["components"]){
            
            if (!component.contains("type")) continue;
        
            const std::string& type = component["type"];

            if(type == "transform"){
                a->GetComponent<ECS::Components::Transform>()->Deserialize(component, levelData);
            }
            else if(type == "model"){
                std::shared_ptr<ECS::Components::Model> model = a->AddComponent<ECS::Components::Model>();
                model->Deserialize(component, levelData);
            }
            else if(type == "light"){
                std::shared_ptr<ECS::Components::Light> light = a->AddComponent<ECS::Components::Light>();
                light->Deserialize(component, levelData);
            }
            else if(type == "physics_body"){
                std::shared_ptr<ECS::Components::PhysicsBody> body = a->AddComponent<ECS::Components::PhysicsBody>();
                body->Deserialize(component, levelData);
            }
            else if(type == "camera"){
                std::shared_ptr<ECS::Components::Camera> cam = a->AddComponent<ECS::Components::Camera>();
                cam->Deserialize(component, levelData);
            }
            else if(type == "audio"){
                std::shared_ptr<ECS::Components::AudioSource> audio = a->AddComponent<ECS::Components::AudioSource>();
                audio->Deserialize(component, levelData);
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
                rawComponent->Deserialize(component, levelData);
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

        DeserializeComponents(a, data, actor);
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
                    std::shared_ptr<Rendering::Shader> shader = Core::GetEngine().GetResourcesManager()->GetShader("shaders\\skybox\\skybox");
                    std::shared_ptr<Rendering::Cubemap> cubemap = Core::GetEngine().GetResourcesManager()->GetCubemap(data["skybox"]);
                    
                    if(shader != nullptr && cubemap != nullptr)
                    {
                        std::shared_ptr<ECS::Objects::Skybox> sb = ECS::Objects::Object::Create<ECS::Objects::Skybox>(cubemap, shader);
                        sb->SetShader(shader);
                        sb->SetCubemap(cubemap);
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

    void SerializeActor(std::shared_ptr<Pulse::Engine::ECS::Objects::Actor> a, ordered_json* actorsArray, ordered_json* meshes, ordered_json* materials){
        
        ordered_json actor;

        actor["name"] = a->GetName();



        for(auto& childrenID : a->GetChildrenID(false)){
            auto child = a->GetChild(childrenID);
            SerializeActor(std::dynamic_pointer_cast<ECS::Objects::Actor>(child), &actor["children"], meshes, materials);
        }

        for(auto& comp : a->GetComponents()){
            ordered_json serializedComp = comp->Serialize();
            if (serializedComp.contains("meshes") && serializedComp["meshes"].is_object()) {
                for (auto& item : serializedComp["meshes"].items()) {
                    if (!meshes->contains(item.key())) {
                        (*meshes)[item.key()] = item.value();
                    }
                }
            }
            
            if (serializedComp.contains("materials") && serializedComp["materials"].is_object()) {
                for (auto& item : serializedComp["materials"].items()) {
                    if (!materials->contains(item.key())) {
                        (*materials)[item.key()] = item.value();
                    }
                }
            }

            if(serializedComp.contains("component")){
                actor["components"].push_back(serializedComp["component"]);
            }
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

        for(auto& actor : rootActors){
            SerializeActor(actor, &actorsArray, &meshes, &materials);
        }

        full["actors"] = actorsArray;

        full["materials"] = materials;

        full["meshes"] = meshes;

        if(skybox){
            full["skybox"] = Core::GetEngine().GetFileManager()->GetFileInfos(skybox->GetCubemap()->GetInfos()->filepath->full).nameInProject;
        }

        std::string fileContent = full.dump();

        filePath.WriteFile(fileContent);
    }

    void Level::SetBuildIndex(int buildIndex)
    {
        this->buildIndex = buildIndex;
    }

    void Level::Clear()
    {
        for(auto& script : scripts){
            if(script->Active()){
                script->OnDestroyed();
            }
        }
        
        for(auto& actor : rootActors){
            RemoveActor(actor->GetID());
        }

        lights.clear();
        transforms.clear();
        models.clear();
        physicsBodies.clear();
        audioSources.clear();
        cameras.clear();
        scripts.clear();
        meshes.clear();
    }

    void Level::OnLoad()
    {
        for(auto& cam : cameras){
            Core::GetEngine().GetCameraManager()->AddCamera(cam->parent->GetID(), cam);
        }

        for(int i = 0; i < lights.size(); i++){
            lights[i]->SetLightIndex(i);
        }

        for(auto& model : models){
            model->Update();
        }

        for(auto& script : scripts){
            script->OnLevelLoaded(); 
        }
    }

    void Level::Unload()
    {
        for(auto& script : scripts){
            script->OnLevelUnloaded();
        }

        loaded = false;
        
        Clear();
    }

    void Level::Tick()
    {
        for(auto& script : scripts){
            if(script->Active()){
                if(script->beginCalled)
                    script->Tick();
                else
                    script->Begin();
                    script->beginCalled = true;
            }
        }
    }

    void Level::Begin()
    {
        //TODO
    }

    void Level::AddActor(std::shared_ptr<ECS::Objects::Actor> a)
    {
        rootActors.push_back(a);
        a->SetLevel(this);
    }

    void Level::RemoveActor(ECS::ObjectID id)
    {
        GetActor(id, false)->Destroy();
    }

    std::shared_ptr<ECS::Objects::Actor> Level::GetActor(ECS::ObjectID id, bool recursive)
    {
        for (auto& actorPtr : rootActors)
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

        for (auto& actorPtr : rootActors)
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
}