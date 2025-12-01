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
            level->lights.push_back(light);
        }

        if (auto model = std::dynamic_pointer_cast<Model>(component)) {
            level->models.push_back(model);
        }

        if (auto transform = std::dynamic_pointer_cast<Transform>(component)) {
            level->transforms.push_back(transform);
        }

        if (auto physics = std::dynamic_pointer_cast<PhysicsBody>(component)) {
            level->physicsBodies.push_back(physics);
        }

        if (auto audio = std::dynamic_pointer_cast<AudioSource>(component)) {
            level->audioSources.push_back(audio);
        }

        if (auto script = std::dynamic_pointer_cast<Script>(component)) {
            level->scripts.push_back(script);

            uint32_t id = GetComponentIDInScene(components.size() - 1);

            Core::GetEngine().GetEventDispatcher()->subscribeToComponent<Events::ContactAddedEvent>(id, [script](const Events::ContactAddedEvent& event) {
                script->OnContactAdded(event);
            });
            Core::GetEngine().GetEventDispatcher()->subscribeToComponent<Events::ContactPersistedEvent>(id, [script](const Events::ContactPersistedEvent& event) {
                script->OnContactPersisted(event);
            });
            Core::GetEngine().GetEventDispatcher()->subscribeToComponent<Events::ContactRemovedEvent>(id, [script](const Events::ContactRemovedEvent& event) {
                script->OnContactEnded(event);
            });
        }

        if (auto cam = std::dynamic_pointer_cast<Camera>(component)) {
            level->cameras.push_back(cam);
        }

        return component;
    }

    void Actor::Destroy()
    {
        for(auto& component : components){
            component->Destroy();
        }
        
        Object::Destroy();

        if(level->IsLoaded()){
            int levelBuildIndex = level->GetBuildIndex();
            int levelAssetID = Core::GetEngine().GetAssetIDManager()->GetIDFromNameInProject(Core::GetEngine().GetBuildSettings()->buildIndex[levelBuildIndex].full).GetAsInt();

            Core::GetEngine().GetEventDispatcher()->emitGlobal(Events::LevelStructureChangedEvent(
                                                    levelAssetID, Events::DESTROYED, name, GetID()));
        }

    }

    void Actor::AddChild(std::shared_ptr<Object> o)
    {
        Object::AddChild(o);
        if (std::shared_ptr<Actor> actorChild = std::dynamic_pointer_cast<Actor>(o)) {
            actorChild->SetLevel(this->level);
        }
    }

    void Actor::SetLevel(Levels::Level* lvl)
    {
        if(!lvl) return;

        bool firstTime = true;

        if(this->level)
            firstTime = false;

        this->level = lvl;

        if(firstTime)
            this->level->transforms.push_back(this->transform);
        

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
        std::shared_ptr<Actor> copy = Object::Create<Actor>("Copy of "+name);

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
                    level->transforms.push_back(tr);
            }
            
            if(!copy->level || !copy->level->IsLoaded())
                continue;

            if(std::shared_ptr<Components::AudioSource> audioSource = std::dynamic_pointer_cast<Components::AudioSource>(cloneComp)){
                level->audioSources.push_back(audioSource);
                audioSource->Update();
            }
            else if(std::shared_ptr<Components::Script> script = std::dynamic_pointer_cast<Components::Script>(cloneComp)){    
                level->scripts.push_back(script);
                RegisterComponentEvents(script);
            }
            else if(std::shared_ptr<Components::Camera> camera = std::dynamic_pointer_cast<Components::Camera>(cloneComp)){
                level->cameras.push_back(camera);
                Core::GetEngine().GetCameraManager()->AddCamera(copy->GetID(), camera);
            }
            else if(std::shared_ptr<Components::Light> light = std::dynamic_pointer_cast<Components::Light>(cloneComp)){
                light->SetLightIndex(level->lights.size());
                level->lights.push_back(light);
            }
            else if(std::shared_ptr<Components::Model> model = std::dynamic_pointer_cast<Components::Model>(cloneComp))
            {
                level->models.push_back(model);
                model->Update();
            }
            else if(std::shared_ptr<Components::PhysicsBody> physicsBody = std::dynamic_pointer_cast<Components::PhysicsBody>(cloneComp)){
                level->physicsBodies.push_back(physicsBody);
                physicsBody->CreateBody(physicsBody->GetShapeType(), physicsBody->GetScale(), physicsBody->GetMotionType());
            }
        }

        return copy;
    }
}