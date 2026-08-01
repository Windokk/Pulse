#include "object.hpp"

#include "engine/core/engine.hpp"

namespace Pulse::Engine::Core{

    void Object::AssignObjectID(std::shared_ptr<Object> obj)
    {
        AssignObjectID(obj, &Core::GetEngine());
    }

    void Object::AssignObjectID(std::shared_ptr<Object> obj, IEngineContext* engine)
    {
        obj->id = engine->GetObjectIDManager()->GenerateNewID();
        engine->GetObjectIDManager()->AssignID(obj->id, obj);
    }

    void Object::Destroy()
    {
        GetEngine().GetObjectIDManager()->DestroyID(id); 
    }
}