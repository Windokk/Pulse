#include "object.hpp"

#include "engine/core/engine.hpp"

namespace Pulse::Engine::Core{

    void Object::AssignObjectID(std::shared_ptr<Object> obj)
    {
        obj->id = Core::GetEngine().GetObjectIDManager()->GenerateNewID(); 
        Core::GetEngine().GetObjectIDManager()->AssignID(obj->id, obj);
    }
}