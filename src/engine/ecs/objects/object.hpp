#pragma once

#include <vector>
#include <memory>

#include "engine/ecs/objectID.hpp"


namespace Epoch::Engine::ECS::Objects{
    
    class Object : public std::enable_shared_from_this<Object>{
        public:
        
            static void AssignObjectID(std::shared_ptr<Object> obj);
            
            template <typename T, typename... Args>
            static std::shared_ptr<T> Create(Args&&... args){
                static_assert(std::is_base_of<Object, T>::value, "T must derive from Object");
                std::shared_ptr<T> obj = std::make_shared<T>(std::forward<Args>(args)...);
                AssignObjectID(obj);
                return obj;
            }

            virtual ~Object();

            std::shared_ptr<Object> GetChild(int index);


            std::shared_ptr<Object> GetChild(ObjectID id);
            std::vector<ObjectID> GetChildrenID(bool recursive = false){ 
                std::vector<ObjectID> ids;

                ids.insert(ids.end(), children.begin(), children.end());
                
                if (recursive) {
                    for (const auto& childID : children) {
                        std::shared_ptr<Object> child = GetChild(childID);
                        if (child) {
                            std::vector<ObjectID> subChildren = child->GetChildrenID(true);
                            ids.insert(ids.end(), subChildren.begin(), subChildren.end());
                        }
                    }
                }
                
                return ids;
            }
            int GetChildrenCount() { return children.size(); }

            virtual void AddChild(std::shared_ptr<Object> o);

            void DeleteChildRef(ObjectID child){
                for(int i = 0; i < children.size(); i++){
                    if(children[i].GetAsInt() == child.GetAsInt()){
                        children.erase(children.begin()+i);
                    }
                }
            }

            std::shared_ptr<Object> GetParent();
            void SetParent(ObjectID parentID) { this->parent = parentID; }

            ObjectID GetID() { return id; }

            virtual void Destroy();
        
        private:

            Object();
            ObjectID id;
            std::vector<ObjectID> children;
            ObjectID parent = ObjectIDBuilder().WithValue(-1).Build();

            friend class Actor;
    };
}