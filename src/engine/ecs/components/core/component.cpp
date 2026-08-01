#include "component.hpp"

#include "engine/debugging/logger.hpp"
#include "engine/ecs/objects/actors/actor.hpp"

namespace Pulse::Engine::ECS::Components {

    Component::Component(std::shared_ptr<Objects::Actor> parent, uint32_t local_id)
    {
        this->parent = parent;
        this->local_id = local_id;
    }

    Component::~Component()
    {
    }

    Core::IEngineContext* Component::GetEngineContext() const
    {
        return parent ? parent->GetEngineContext() : nullptr;
    }
}