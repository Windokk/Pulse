#include "component.hpp"

#include "engine/debugging/logger.hpp"

namespace Pulse::Engine::ECS::Components {

    Component::Component(std::shared_ptr<Objects::Actor> parent, uint32_t local_id)
    {
        this->parent = parent;
        this->local_id = local_id;
    }

    Component::~Component()
    {
    }
}