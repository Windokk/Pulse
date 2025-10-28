#include "component.hpp"

namespace Epoch::Engine::ECS::Components {

    Component::Component(std::shared_ptr<Objects::Actor> parent, uint32_t local_id)
    {
        this->parent = parent;
        this->local_id = local_id;
    }

    Component::~Component()
    {
    }

    void Component::Destroy() {
        
    }
}