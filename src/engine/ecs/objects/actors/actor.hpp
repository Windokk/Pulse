#pragma once

#include "engine/ecs/objects/level_object.hpp"

#include "engine/ecs/components/misc/transform.hpp"
#include "engine/ecs/components/rendering/light_component.hpp"
#include "engine/ecs/components/physics/physics_body.hpp"

#include "engine/rendering/camera/camera_manager.hpp"

#include "engine/levels/level.hpp"

#include "engine/events/event_system.hpp"

#include <stdexcept>
#include <iostream>

namespace Pulse::Engine::ECS::Objects{

    using namespace Components;

    class Actor : public LevelObject{
        
        std::vector<std::shared_ptr<Component>> components;        
        std::string name;
        public:
            Actor(std::string name);

            void Init();

            std::shared_ptr<Component> AddComponentRaw(std::shared_ptr<Component> component);

            template <typename T>
            bool HasComponent();

            template <typename T>
            std::vector<std::shared_ptr<T>> GetComponents();

            template <typename T>
            std::shared_ptr<T> GetComponent(int k = 0);

            const std::vector<std::shared_ptr<Component>>& GetComponents() const {
                return components;
            }

            template <typename T>
            std::shared_ptr<T> AddComponent();

            void Destroy() override;

            int GetComponentIDInScene(int componentIndex) { 
                if(componentIndex >= 0 && id.GetAsInt() >= 0){
                    return (id.GetAsInt() << 12) | (componentIndex & 0xFFF);
                }
                DEBUG_ERROR("Either local component ID or actor id is not greater than 0. Returning default value (0).");
                return 0;
            }
            
            std::string GetName() { return name; }
            void SetName(std::string name) { this->name = name; }

            void AddChild(std::shared_ptr<LevelObject> o) override;

            void SetLevel(Levels::Level* lvl);

            void RegisterComponentEvents(const std::shared_ptr<Script>& component);

            std::shared_ptr<Transform> transform = nullptr;
            Levels::Level* level = nullptr;

            void Activate();

            void DeActivate();

            bool IsActive(){
                return activated;
            }

            std::shared_ptr<Actor> Clone();

            // TODO : SetParent (Attach to)

        protected:
            bool activated = true;
    };

    

    template <typename T>
    bool Actor::HasComponent() {
        if(!std::is_base_of<Component, T>::value){
            DEBUG_ERROR("T must inherit from Component");
        }

        for (const std::shared_ptr<Component>& component : components) {
            if (dynamic_cast<T*>(component.get())) {
                return true;
            }
        }
        return false;
    }

    template <typename T>
    std::vector<std::shared_ptr<T>> Actor::GetComponents(){
        if(!std::is_base_of<Component, T>::value){
            DEBUG_ERROR("T must inherit from Component");
        }

        std::vector<std::shared_ptr<T>> list;

        for (const auto& component : components) {
            if (auto casted = std::dynamic_pointer_cast<T>(component)) {
                list.push_back(casted);
            }
        }

        return list;
    } 

    template <typename T>
    std::shared_ptr<T> Actor::GetComponent(int k) {
        if(!std::is_base_of<Component, T>::value){
            DEBUG_ERROR("T must inherit from Component");
        }

        int n = 0;

        for (auto& component : components) {
            if (auto casted = std::dynamic_pointer_cast<T>(component)) {
                if(n != k){
                    n++;
                    continue;
                }
                else{
                    return casted;
                }
            }
        }
        DEBUG_ERROR(std::string("Failed to retrieve Component of type ") + typeid(T).name());
        return nullptr;
    }

    template <typename T>
    std::shared_ptr<T> Actor::AddComponent()
    {
        if(!IsSubclassOf<Component, T>()){
            DEBUG_ERROR("T must inherit from Component");
            return nullptr;
        }

        std::shared_ptr<T> component = Object::Create<T>(AsShared<Actor>(), components.size());

        if (transform != nullptr && IsSubclassOf<Transform, T>()) {
            DEBUG_ERROR("An actor can only have one transform component.");
            return nullptr;
        }

        components.push_back(component);

        if(level != nullptr){
            if constexpr (IsSubclassOf<Light, T>()) {
                component->SetLightIndex(level->lights.size());
                level->lights.push_back(component);
                level->lightComps.emplace(GetComponentIDInScene(component->GetLocalId()), component);
            }

            if constexpr (IsSubclassOf<Model, T>()) {
                level->models.emplace(GetComponentIDInScene(component->GetLocalId()), component);
            }

            if constexpr (IsSubclassOf<Transform, T>()) {
                level->transforms.emplace(GetComponentIDInScene(component->GetLocalId()), component);
            }

            if constexpr (IsSubclassOf<PhysicsBody, T>()) {
                level->physicsBodies.emplace(GetComponentIDInScene(component->GetLocalId()), component);
            }

            if constexpr (IsSubclassOf<AudioSource, T>()) {
                level->audioSources.emplace(GetComponentIDInScene(component->GetLocalId()), component);
            }

            if constexpr (IsSubclassOf<Script, T>()) {
                level->scripts.emplace(GetComponentIDInScene(component->GetLocalId()), component);
                RegisterComponentEvents(component);
                component->OnCreate();
            }

            if constexpr (IsSubclassOf<Camera, T>()) {
                level->cameras.emplace(GetComponentIDInScene(component->GetLocalId()), component);
            }
        }

        return component;
    }
}