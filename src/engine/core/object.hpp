#pragma once

#include "engine/core/objectID.hpp"

namespace Pulse::Engine::Core{

    class FieldChangedEvent;

    class Object {
        public:
            virtual ~Object() = default;

            ObjectID GetID() const { return id; }

            static void AssignObjectID(std::shared_ptr<LevelObject> obj);
            
            template <typename T, typename... Args>
            static std::shared_ptr<T> Create(Args&&... args){
                static_assert(std::is_base_of<Object, T>::value, "T must derive from Object");
                std::shared_ptr<T> obj = std::make_shared<T>(std::forward<Args>(args)...);
                AssignObjectID(obj);
                Object::CallInit(obj);
                return obj;
            }

            // Reflection / editor
            virtual void OnFieldChanged(const FieldChangedEvent&) {}
            virtual void OnBeginEdit() {}
            virtual void OnEndEdit() {}

        protected:
            Object();

        private:
            ObjectID id;
    };
}