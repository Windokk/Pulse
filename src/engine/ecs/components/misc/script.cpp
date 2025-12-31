#include "script.hpp"

#include "script.reflection.hpp"

namespace Pulse::Engine::ECS::Components
{
    Script::Script(std::shared_ptr<Objects::Actor> parent, uint32_t local_id) : Component(parent, local_id)
    {    
    }

    void Script::OnLevelLoaded()
    {
    }

    void Script::OnLevelUnloaded()
    {
    }

    void Script::Begin()
    {
    }

    void Script::Tick()
    {
    }

    void Script::OnDestroyed()
    {
    }

    void Script::OnContactAdded(const Events::ContactAddedEvent& event)
    {
    }

    void Script::OnContactPersisted(const Events::ContactPersistedEvent& event)
    {
    }

    void Script::OnContactEnded(const Events::ContactRemovedEvent& event)
    {
    }

    void Script::OnActivated()
    {
    }

    void Script::OnDeactivated()
    {

    }

    std::shared_ptr<Component> Script::Clone() const
    {
        auto cloned = std::make_shared<Script>(*this);

        return cloned;
    }
}
