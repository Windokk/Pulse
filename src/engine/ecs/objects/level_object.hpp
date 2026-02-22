#pragma once

#include <vector>
#include <memory>

#include "engine/core/object.hpp"


namespace Pulse::Engine::ECS::Objects{
    
    class LevelObject : public std::enable_shared_from_this<LevelObject>{
        public:

            virtual ~LevelObject();

            std::shared_ptr<LevelObject> GetChild(int index);

            std::shared_ptr<LevelObject> GetChild(ObjectID id);
            std::vector<ObjectID> GetChildrenID(bool recursive = false){ 
                std::vector<ObjectID> ids;

                ids.insert(ids.end(), children.begin(), children.end());
                
                if (recursive) {
                    for (const auto& childID : children) {
                        std::shared_ptr<LevelObject> child = GetChild(childID);
                        if (child) {
                            std::vector<ObjectID> subChildren = child->GetChildrenID(true);
                            ids.insert(ids.end(), subChildren.begin(), subChildren.end());
                        }
                    }
                }
                
                return ids;
            }
            int GetChildrenCount() { return children.size(); }

            virtual void AddChild(std::shared_ptr<LevelObject> o);

            void DeleteChildRef(ObjectID child){
                for(int i = 0; i < children.size(); i++){
                    if(children[i].GetAsInt() == child.GetAsInt()){
                        children.erase(children.begin()+i);
                    }
                }
            }

            std::shared_ptr<LevelObject> GetParent();
            void SetParent(ObjectID parentID) { this->parent = parentID; }

            ObjectID GetID() { return id; }

            virtual void Destroy();
        
        private:

            template <typename, typename = std::void_t<>>
            struct HasInit : std::false_type {};

            template <typename T>
            struct HasInit<T, std::void_t<decltype(std::declval<T>().Init())>> : std::true_type {};

            // Primary template: HasInit<T> == true
            template <typename U>
            static std::enable_if_t<HasInit<U>::value>
            CallInit(std::shared_ptr<U> obj) {
                obj->Init();
            }

            // Fallback: HasInit<T> == false
            template <typename U>
            static std::enable_if_t<!HasInit<U>::value>
            CallInit(std::shared_ptr<U>) {
                // do nothing
            }

            LevelObject();
            ObjectID id;
            std::vector<ObjectID> children;
            ObjectID parent = ObjectID(-1);

            friend class Actor;
            friend class Skybox;
    };
}