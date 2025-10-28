#include "object.hpp"

#include "engine/debugging/debugger.hpp"

#include "engine/core/engine.hpp"

namespace Epoch::Engine::ECS::Objects{
    
    std::shared_ptr<Object> Object::GetParent()
    {
        return Core::GetEngine().GetObjectIDManager()->GetObjectFromID(parent);
    }

    void Object::Destroy()
    {
        if(parent.GetAsInt() != -1){
            Core::GetEngine().GetObjectIDManager()->GetObjectFromID(parent)->DeleteChildRef(id);
        }
        for(auto& child : children)
        { 
            Core::GetEngine().GetObjectIDManager()->GetObjectFromID(child)->Destroy(); 
        }
        children.clear();
        Core::GetEngine().GetObjectIDManager()->DestroyID(id); 
    }

    Object::Object()
    {

    }

    void Object::AssignObjectID(std::shared_ptr<Object> obj)
    {
        obj->id = Core::GetEngine().GetObjectIDManager()->GenerateNewID();
        Core::GetEngine().GetObjectIDManager()->AssignID(obj->id, obj);
    }

    Object::~Object()
    {

    }

    std::shared_ptr<Object> Object::GetChild(int index)
    {
        return Core::GetEngine().GetObjectIDManager()->GetObjectFromID(children[index]);
    }

    std::shared_ptr<Object> Object::GetChild(ObjectID ObjectID)
    {
        if (GetID() == ObjectID)
            return shared_from_this();

        for (auto& child : children)
        {
            if (child == ObjectID)
                return Core::GetEngine().GetObjectIDManager()->GetObjectFromID(child);
        }

        return nullptr;
    }

    void Object::AddChild(std::shared_ptr<Object> o)
    {
        children.push_back(o->GetID());
        o->SetParent(id);
    }
}