#include "component_registry.hpp"

#include <iostream>

#include "engine/debugging/debugger.hpp"

namespace Epoch::Engine::ECS::Components {

    ComponentRegistry gSharedComponentRegistry;

    void ComponentRegistry::RegisterComponentType(const std::string& name, ComponentFactory factory) {
        if (registry.find(name) != registry.end()) {
            DEBUG_ERROR("Component already registered: " + name);
        }
        registry[name] = factory;
        DEBUG_INFO("Registered custom component : " + name);
    }

    Component* ComponentRegistry::CreateComponentByName(const std::string& name) {
        auto it = registry.find(name);
        if (it == registry.end()) {
            DEBUG_ERROR("Component not registered: " + name);
            return nullptr;
        }
        return it->second();
    }

    const std::unordered_map<std::string, ComponentFactory>& ComponentRegistry::GetAll() const {
        return registry;
    }
}