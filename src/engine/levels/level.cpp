#include "level.hpp"

#include <iostream>
#include <algorithm>

#include "engine/objects/actors/actor.hpp"
#include "engine/objects/components/core/registry/component_registry.hpp"
#include "engine/objects/components/audio/audio_source.hpp"
#include "engine/core/engine.hpp"
#include "engine/core/resources/resources_manager.hpp"
#include "engine/objects/skybox/skybox.hpp"
#include "engine/rendering/texture/cubemap/envmap.hpp"
#include "engine/rendering/material/material.hpp"
#include "engine/rendering/pipeline/pipeline.hpp"
#include "engine/rendering/renderer/renderer.hpp"

namespace Pulse::Engine::Levels{


    Level::Level(std::string name, Filesystem::Path path)
    {
        this->name = name;
        this->path = path;
    }

    void DeserializeComponents(std::shared_ptr<Objects::Actor> a, json actorData){
        if(!actorData["components"].is_array()){
            DEBUG_ERROR("Couldn't deserialize actor's components as actor[components] is not an array");
            return; 
        }

        for(auto& component : actorData["components"]){
            
            if (!component.contains("type")) continue;
        
            const std::string& type = component["type"];

            if(type == "transform"){
                a->GetComponent<Objects::Components::Transform>()->Deserialize(component);
            }
            else if(type == "model"){
                std::shared_ptr<Objects::Components::Model> model = a->AddComponent<Objects::Components::Model>();
                model->Deserialize(component);
            }
            else if(type == "light"){
                std::shared_ptr<Objects::Components::Light> light = a->AddComponent<Objects::Components::Light>();
                light->Deserialize(component);
            }
            else if(type == "physics_body"){
                std::shared_ptr<Objects::Components::PhysicsBody> body = a->AddComponent<Objects::Components::PhysicsBody>();
                body->Deserialize(component);
            }
            else if(type == "camera"){
                std::shared_ptr<Objects::Components::Camera> cam = a->AddComponent<Objects::Components::Camera>();
                cam->Deserialize(component);
            }
            else if(type == "audio"){
                std::shared_ptr<Objects::Components::AudioSource> audio = a->AddComponent<Objects::Components::AudioSource>();
                audio->Deserialize(component);
            }
            else if(type == "probeVolume"){
                std::shared_ptr<Objects::Components::ProbeVolume> probeVolume = a->AddComponent<Objects::Components::ProbeVolume>();
                probeVolume->Deserialize(component);
            }
            else{
                //Custom component/Inherited component case
                //Note : The custom component has to be already registered
                std::shared_ptr<Objects::Components::Component> rawComponent = Pulse::Engine::Objects::Components::GetComponentRegistry().CreateComponentByName(type);
                if (!rawComponent) {
                    DEBUG_WARNING("Unknown component type: " + type);
                    continue;
                }

                a->AddComponentRaw(rawComponent);
                rawComponent->Deserialize(component);
            }
        }
    }

    void DeserializeActor(std::shared_ptr<Objects::Actor> a, json data, json actor){

        if (actor.contains("children") && actor["children"].is_array() && !actor["children"].empty()) {
            Core::IEngineContext* engine = &Core::GetEngine();
            for (auto& child : actor["children"]) {
                std::shared_ptr<Objects::Actor> b = Core::Object::CreateWithContext<Objects::Actor>(engine, child["name"], engine);
                a->AddChild(b);
                DeserializeActor(b, data, child);
            }
        }

        DeserializeComponents(a, actor);
    }

    static void CollectActorAssetRefs(const json& actor, LevelAssetManifest& manifest){

        if (actor.contains("children") && actor["children"].is_array()) {
            for (auto& child : actor["children"]) {
                CollectActorAssetRefs(child, manifest);
            }
        }

        if (!actor.contains("components") || !actor["components"].is_array())
            return;

        for (auto& component : actor["components"]) {

            if (!component.contains("type") || !component["type"].is_string())
                continue;

            const std::string& type = component["type"];

            if (type == "model") {
                if (component.contains("mesh") && component["mesh"].is_string())
                    manifest.meshPathsInProject.push_back(component["mesh"].get<std::string>());

                if (component.contains("materials") && component["materials"].is_object()) {
                    for (auto it = component["materials"].begin(); it != component["materials"].end(); ++it) {
                        if (it.value().is_string())
                            manifest.materialPathsInProject.push_back(it.value().get<std::string>());
                    }
                }
            }
        }
    }

    LevelAssetManifest CollectLevelAssetRefs(const Filesystem::Path& filePath)
    {
        LevelAssetManifest manifest;

        std::string src = filePath.ReadFile();

        try {
            json data = json::parse(src);

            if (data.contains("actors") && data["actors"].is_array()) {
                for (auto& actor : data["actors"]) {
                    CollectActorAssetRefs(actor, manifest);
                }
            }

            if (data.contains("skybox") && data["skybox"].is_string()) {
                manifest.skyboxEnvMapPathInProject = data["skybox"].get<std::string>();
            }

            manifest.success = true;

        } catch (const json::parse_error& e) {
            DEBUG_ERROR("JSON parse error: " + (std::string)e.what());
        }

        return manifest;
    }

    void Level::Deserialize(Filesystem::Path filePath)
    {
        std::string src = filePath.ReadFile();

        try {
            json data = json::parse(src);

            name = data["name"];

            Core::IEngineContext* engine = &Core::GetEngine();
            for(auto& actor : data["actors"])
            {
                std::shared_ptr<Objects::Actor> a = Core::Object::CreateWithContext<Objects::Actor>(engine, actor["name"], engine);
                AddActor(a);
                DeserializeActor(a, data, actor);
            }

            if(data.contains("skybox")){
                auto& skybox_folder = data["skybox"];
                if(skybox_folder.is_string()){
                    std::shared_ptr<Rendering::Shader> shader = Core::GetEngine().GetResourcesManager()->GetShader("shaders/skybox/skybox");
                    std::shared_ptr<Rendering::EnvironmentMap> envMap = Core::GetEngine().GetResourcesManager()->GetEnvMap(data["skybox"]);
                    Rendering::PipelineSpecifications specs;
                    specs.depthCompare = Rendering::DepthCompareOp::LessOrEqual;
                    specs.depthWrite = false;
                    specs.shader = shader;
                    specs.topology = Rendering::PrimitiveTopology::Triangles;
                    specs.debugName = "SkyboxPipeline";

                    std::shared_ptr<Rendering::Pipeline> skyboxPipeline = Core::GetEngine().GetRenderer()->GetOrAddPipeline(specs);

                    std::shared_ptr<Rendering::Material> skyboxMat = Rendering::Material::Create(shader, skyboxPipeline, false, Rendering::Opacity::Opaque);

                    if(shader != nullptr && envMap != nullptr)
                    {
                        std::shared_ptr<Objects::Skybox> sb = Core::Object::Create<Objects::Skybox>(envMap, skyboxMat);
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

    void SerializeActor(std::shared_ptr<Pulse::Engine::Objects::Actor> a, ordered_json* actorsArray){
        
        ordered_json actor;

        actor["name"] = a->GetName();

        for(auto& childrenID : a->GetChildrenID(false)){
            auto child = a->GetChild(childrenID);
            SerializeActor(std::dynamic_pointer_cast<Objects::Actor>(child), &actor["children"]);
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

        std::vector<std::pair<Core::ObjectID, std::shared_ptr<Objects::Actor>>> sortedActors(
            rootActors.begin(), rootActors.end()
        );

        std::sort(sortedActors.begin(), sortedActors.end(),
                [](const auto& a, const auto& b) { return a.first < b.first; });

        for(auto& [id, actorPtr] : sortedActors){
            SerializeActor(actorPtr, &actorsArray);
        }

        full["actors"] = actorsArray;

        if(skybox){
            full["skybox"] = Core::GetEngine().GetFileManager()->GetFileInfos(Core::GetEngine().GetAssetIDManager()->GetAssetFromID(skybox->GetEnvMap()->GetAssetID())->baseInfos.path).nameInProject;
        }

        std::string fileContent = full.dump();

        filePath.WriteFile(fileContent);
    }

    void Level::SetBuildIndex(int buildIndex)
    {
        this->buildIndex = buildIndex;
    }

    void Level::RemoveActorRecursive(Core::ObjectID actorID)
    {
        auto objPtr = Core::GetEngine().GetObjectIDManager()->GetObjectFromID(actorID);
        if (!objPtr && !dynamic_pointer_cast<Objects::LevelObject>(objPtr))
            return;

        auto lvlObjPtr = dynamic_pointer_cast<Objects::LevelObject>(objPtr);

        // Copy children IDs FIRST
        std::vector<Core::ObjectID> children;
        children.reserve(lvlObjPtr->GetChildrenCount());

        for (int i = 0; i < lvlObjPtr->GetChildrenCount(); ++i)
            children.push_back(lvlObjPtr->GetChild(i)->GetID());

        // Now safely recurse
        for (Core::ObjectID childID : children)
            RemoveActorRecursive(childID);

        // Finally remove this actor
        auto actorPtr = std::dynamic_pointer_cast<Objects::Actor>(objPtr);
        if (!actorPtr)
            return;

        actorPtr->Destroy();
    }

    void Level::Clear()
    {
        // Make a copy of keys because RemoveActorRecursive modifies rootActors
        std::vector<Core::ObjectID> rootIDs;
        for (auto& [id, actorPtr] : rootActors)
            rootIDs.push_back(id);

        for (Core::ObjectID id : rootIDs)
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
        
        if(probeVolume)
            probeVolume->Activate();
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

    void Level::Stop()
    {
        for(auto& [id,script] : scripts){
            if(script->Active()){
                script->OnStop();
            }
        }
    }

    void Level::AddActor(std::shared_ptr<Objects::Actor> a)
    {
        rootActors.emplace(a->GetID(), a);
        a->SetLevel(this);
    }

    void Level::RemoveActor(Core::ObjectID id)
    {
        if(rootActors.find(id) != rootActors.end())
            rootActors.erase(id);
    }

    std::shared_ptr<Objects::Actor> Level::GetActor(Core::ObjectID id, bool recursive)
    {
        for (auto& [id, actorPtr] : rootActors)
        {
            if (actorPtr->GetID() == id)
                return actorPtr;

            if(recursive){
                std::vector<Core::ObjectID> children = actorPtr->GetChildrenID(true);

                for(auto& _id : children){
                    std::shared_ptr<Objects::Actor> child = std::dynamic_pointer_cast<Objects::Actor>(Core::GetEngine().GetObjectIDManager()->GetObjectFromID(_id));
                    if(_id == id && child){
                        return child;
                    }
                }
            }
        }

        return nullptr;
    }

    std::vector<Core::ObjectID> Level::GetActorsID(bool recursive)
    {
        std::vector<Core::ObjectID> actorIDs;

        for (auto& [id, actorPtr] : rootActors)
        {
            actorIDs.push_back(actorPtr->GetID());

            if(recursive){
                std::vector<Core::ObjectID> children = actorPtr->GetChildrenID(true);
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

    void Level::RemoveComponent(const int idInLevel, const std::shared_ptr<Objects::Components::Component> compPtr)
    {
        compPtr->Destroy();
        if(compPtr->IsInstanceOf<Objects::Components::AudioSource>()){
            audioSources.erase(idInLevel);
        }
        else if(compPtr->IsInstanceOf<Objects::Components::Transform>()){
            transforms.erase(idInLevel);
        }
        else if(compPtr->IsInstanceOf<Objects::Components::PhysicsBody>()){
            physicsBodies.erase(idInLevel);
        }
        else if(compPtr->IsInstanceOf<Objects::Components::Camera>()){
            cameras.erase(idInLevel);
        }
        else if(compPtr->IsInstanceOf<Objects::Components::Script>()){
            scripts.erase(idInLevel);
        }
        else if(compPtr->IsInstanceOf<Objects::Components::Light>()){
            int index = lightComps.at(idInLevel)->GetLightIndex();
            std::rotate(lights.begin() + index, lights.begin() + index + 1, lights.end());
            lights.pop_back();
            lightComps.erase(idInLevel);
        }
        else if(compPtr->IsInstanceOf<Objects::Components::Model>()){
            meshes.erase(idInLevel);
        }
    }
}