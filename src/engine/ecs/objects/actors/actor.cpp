#include "actor.hpp"

#include "engine/core/engine.hpp"

namespace Pulse::Engine::ECS::Objects{
    
    using namespace Components;

    Actor::Actor(std::string name)
    {
        SetName(name);
    }

    void Actor::Init(){
        this->transform = AddComponent<Transform>();
    }

    std::shared_ptr<Component> Actor::AddComponentRaw(Component* rawComponent) {
        if (!rawComponent) {
            DEBUG_ERROR("Tried to add null component.");
        }

        // Wrap in shared_ptr
        std::shared_ptr<Component> component(rawComponent);

        // Set actor and component index
        component->SetParent(std::static_pointer_cast<Actor>(shared_from_this()));
        component->SetLocalId(components.size());

        if (dynamic_cast<Transform*>(component.get())) {
            DEBUG_ERROR("An actor can only have one transform component.");
        }

        components.push_back(component);

        // Register in system component arrays
        if (auto light = std::dynamic_pointer_cast<Light>(component)) {
            light->SetLightIndex(level->lights.size());
            level->lights.push_back(light);
            level->lightComps.emplace(GetComponentIDInScene(light->GetLocalId()), light);
        }

        if (auto model = std::dynamic_pointer_cast<Model>(component)) {
            level->models.emplace(GetComponentIDInScene(model->GetLocalId()), model);
        }

        if (auto transform = std::dynamic_pointer_cast<Transform>(component)) {
            level->transforms.emplace(GetComponentIDInScene(transform->GetLocalId()), transform);
        }

        if (auto physics = std::dynamic_pointer_cast<PhysicsBody>(component)) {
            level->physicsBodies.emplace(GetComponentIDInScene(physics->GetLocalId()), physics);
        }

        if (auto audio = std::dynamic_pointer_cast<AudioSource>(component)) {
            level->audioSources.emplace(GetComponentIDInScene(audio->GetLocalId()), audio);
        }

        if (auto script = std::dynamic_pointer_cast<Script>(component)) {
            level->scripts.emplace(GetComponentIDInScene(script->GetLocalId()), script);

            uint32_t id = GetComponentIDInScene(components.size() - 1);

            RegisterComponentEvents(script);

            script->OnCreate();
        }

        if (auto cam = std::dynamic_pointer_cast<Camera>(component)) {
            level->cameras.emplace(GetComponentIDInScene(cam->GetLocalId()), cam);
        }

        return component;
    }

    void Actor::Destroy()
    {
        for(auto& component : components){
            
            if(level)
                level->RemoveComponent(GetComponentIDInScene(component->GetLocalId()), component);
            
            component->Destroy();
        }

        if(level){

            level->RemoveActor(id);
            
            if(level->IsLoaded()){
                int levelBuildIndex = level->GetBuildIndex();
                int levelAssetID = Core::GetEngine().GetAssetIDManager()->GetIDFromNameInProject(Core::GetEngine().GetBuildSettings()->buildIndex[levelBuildIndex].full).GetAsInt();

                Core::GetEngine().GetEventDispatcher()->emitGlobal(Events::LevelStructureChangedEvent(
                                                        levelAssetID, Events::DESTROYED, name, GetID()));
            }
        }
        
        LevelObject::Destroy();
    }

    void Actor::AddChild(std::shared_ptr<LevelObject> o)
    {
        LevelObject::AddChild(o);
        if (std::shared_ptr<Actor> actorChild = std::dynamic_pointer_cast<Actor>(o)) {
            actorChild->SetLevel(this->level);
        }
    }

    void Actor::SetLevel(Levels::Level* lvl)
    {
        if(!lvl) return;

        this->level = lvl;

        if(level->IsLoaded()){
            int levelBuildIndex = level->GetBuildIndex();
            int levelAssetID = Core::GetEngine().GetAssetIDManager()->GetIDFromNameInProject(Core::GetEngine().GetBuildSettings()->buildIndex[levelBuildIndex].full).GetAsInt();

            Core::GetEngine().GetEventDispatcher()->emitGlobal(Events::LevelStructureChangedEvent(
                                                    levelAssetID, Events::CREATED, name, GetID()));
        }
    }

    void Actor::RegisterComponentEvents(const std::shared_ptr<Script>& component){
        
        Core::GetEngine().GetEventDispatcher()->subscribeToComponent<Events::ContactAddedEvent>(GetComponentIDInScene(components.size()-1), [component](const Events::ContactAddedEvent& event) {
            component->OnContactAdded(event);
        });
        Core::GetEngine().GetEventDispatcher()->subscribeToComponent<Events::ContactPersistedEvent>(GetComponentIDInScene(components.size()-1), [component](const Events::ContactPersistedEvent& event) {
            component->OnContactPersisted(event);
        });
        Core::GetEngine().GetEventDispatcher()->subscribeToComponent<Events::ContactRemovedEvent>(GetComponentIDInScene(components.size()-1), [component](const Events::ContactRemovedEvent& event) {
            component->OnContactEnded(event);
        });
    }

    void Actor::Activate()
    {
        activated = true;
        for(auto& component : components){
            component->Activate();
        }
        if(level->IsLoaded()){
            int levelBuildIndex = level->GetBuildIndex();
            int levelAssetID = Core::GetEngine().GetAssetIDManager()->GetIDFromNameInProject(Core::GetEngine().GetBuildSettings()->buildIndex[levelBuildIndex].full).GetAsInt();

            Core::GetEngine().GetEventDispatcher()->emitGlobal(Events::LevelStructureChangedEvent(
                                                    levelAssetID, Events::ACTIVATED, name, GetID()));
        }
    }

    void Actor::DeActivate()
    {
        activated = false;
        for(auto& component : components){
            component->DeActivate();
        }
        if(level->IsLoaded()){
            int levelBuildIndex = level->GetBuildIndex();
            int levelAssetID = Core::GetEngine().GetAssetIDManager()->GetIDFromNameInProject(Core::GetEngine().GetBuildSettings()->buildIndex[levelBuildIndex].full).GetAsInt();

            Core::GetEngine().GetEventDispatcher()->emitGlobal(Events::LevelStructureChangedEvent(
                                                    levelAssetID, Events::DEACTIVATED, name, GetID()));
        }
    }
    
    std::shared_ptr<Actor> Actor::Clone()
    {
        std::shared_ptr<Actor> copy = Core::Object::Create<Actor>("Copy of "+name);

        DEBUG_INFO("Cloning actor : "+name);

        if(GetParent() && std::dynamic_pointer_cast<Actor>(GetParent())){
            std::shared_ptr<Actor> p = std::dynamic_pointer_cast<Actor>(GetParent());
            GetParent()->AddChild(copy);
        }
        else if(level){
            level->AddActor(copy);
        }
        else{
            DEBUG_INFO("Cloning : Base actor was not placed in a level, clone won't be placed in a level either");
        }

        for(int i = 0; i < components.size(); i++){

            std::shared_ptr<Component> comp = components[i];

            std::shared_ptr<Component> cloneComp = comp->Clone();

            cloneComp->SetParent(copy);

            copy->components.push_back(cloneComp);

            if (cloneComp->IsInstanceOf<Transform>()) {
                
                std::shared_ptr<Transform> tr = std::dynamic_pointer_cast<Components::Transform>(cloneComp);

                tr->SetPosition(transform->GetPosition());
                tr->SetRotation(transform->GetRotation());
                tr->SetScale(transform->GetScale());

                if(copy->level && copy->level->IsLoaded())
                    level->transforms.emplace(GetComponentIDInScene(tr->GetLocalId()), tr);
            }
            
            if(!copy->level || !copy->level->IsLoaded())
                continue;

            if(std::shared_ptr<Components::AudioSource> audioSource = std::dynamic_pointer_cast<Components::AudioSource>(cloneComp)){
                level->audioSources.emplace(GetComponentIDInScene(audioSource->GetLocalId()), audioSource);
                audioSource->Update();
            }
            else if(std::shared_ptr<Components::Script> script = std::dynamic_pointer_cast<Components::Script>(cloneComp)){    
                level->scripts.emplace(GetComponentIDInScene(script->GetLocalId()), script);
                RegisterComponentEvents(script);
                script->OnCreate();
            }
            else if(std::shared_ptr<Components::Camera> camera = std::dynamic_pointer_cast<Components::Camera>(cloneComp)){
                level->cameras.emplace(GetComponentIDInScene(camera->GetLocalId()), camera);
                Core::GetEngine().GetCameraManager()->AddCamera(copy->GetID(), camera);
            }
            else if(std::shared_ptr<Components::Light> light = std::dynamic_pointer_cast<Components::Light>(cloneComp)){
                light->SetLightIndex(level->lights.size());
                level->lights.push_back(light);
                level->lightComps.emplace(GetComponentIDInScene(light->GetLocalId()), light);
            }
            else if(std::shared_ptr<Components::Model> model = std::dynamic_pointer_cast<Components::Model>(cloneComp))
            {
                level->models.emplace(GetComponentIDInScene(model->GetLocalId()), model);
                model->Update();
            }
            else if(std::shared_ptr<Components::PhysicsBody> physicsBody = std::dynamic_pointer_cast<Components::PhysicsBody>(cloneComp)){
                level->physicsBodies.emplace(GetComponentIDInScene(physicsBody->GetLocalId()), physicsBody);
                physicsBody->CreateBody(physicsBody->GetShapeType(), physicsBody->params, physicsBody->GetMotionType());
            }
        }

        return copy;
    }
}