#include "actor.hpp"

#include "engine/core/engine.hpp"

namespace Epoch::Engine::ECS::Objects{
    
    using namespace Components;

    Actor::Actor(std::string name)
    {
        SetName(name);

        transform = make_shared<Transform>(this, this->components.size());
        components.push_back(transform);
    }

    std::shared_ptr<Component> Actor::AddComponentRaw(Component* rawComponent) {
        if (!rawComponent) {
            DEBUG_ERROR("Tried to add null component.");
        }

        // Wrap in shared_ptr
        std::shared_ptr<Component> component(rawComponent);

        // Set actor and component index
        component->parent = this;
        component->local_id = components.size();

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

        if(level){
            int levelBuildIndex = Core::GetEngine().GetBuildSettings()->GetLevelBuildIndex(level->GetName());

            if(levelBuildIndex == -1)
                return;

            Core::GetEngine().GetEventDispatcher()->emitGlobal(Events::LevelStructureChanged(
                                                    levelBuildIndex, Events::DESTROYED, GetID()));
        }

        Object::Destroy();
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

        this->level = lvl;

        if(level){
            int levelBuildIndex = Core::GetEngine().GetBuildSettings()->GetLevelBuildIndex(level->GetName());
            
            if(levelBuildIndex == -1)
                return;

            Core::GetEngine().GetEventDispatcher()->emitGlobal(Events::LevelStructureChanged(
                                                    levelBuildIndex, Events::DESTROYED, GetID()));
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
        if(level){
            int levelBuildIndex = Core::GetEngine().GetBuildSettings()->GetLevelBuildIndex(level->GetName());

            if(levelBuildIndex == -1)
                return;

            Core::GetEngine().GetEventDispatcher()->emitGlobal(Events::LevelStructureChanged(
                                                    levelBuildIndex, Events::DESTROYED, GetID()));
        }
    }

    void Actor::DeActivate()
    {
        activated = false;
        for(auto& component : components){
            component->DeActivate();
        }
        if(level){
            int levelBuildIndex = Core::GetEngine().GetBuildSettings()->GetLevelBuildIndex(level->GetName());

            if(levelBuildIndex == -1)
                return;

            Core::GetEngine().GetEventDispatcher()->emitGlobal(Events::LevelStructureChanged(
                                                    levelBuildIndex, Events::DESTROYED, GetID()));
        }
    }
}