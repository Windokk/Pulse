#pragma once

#include <string>
#include <atomic>
#include <unordered_set>
#include <map>
#include <cstddef>
#include <functional>
#include <memory>

namespace Epoch::Engine::ECS
{
    namespace Objects{
        class Object;
    }

    class ObjectID {
        public:
            
            ObjectID() : packed(0) {}
            explicit ObjectID(int value) : packed(value) {}

            
            int GetAsInt() const {
                return packed;
            }

            std::string GetAsString() const {
                return std::to_string(GetAsInt());
            }

            
            bool operator==(const ObjectID& other) const { return packed == other.packed; }
            bool operator!=(const ObjectID& other) const { return !(*this == other); }
            bool operator<(const ObjectID& other) const { return packed < other.packed; }

            
            friend class ObjectIDBuilder;
    
        private:
            int packed;
    };
    
    class ObjectIDBuilder {
        public:
            ObjectIDBuilder& WithValue(int val) {
                value = val;
                generated = false;
                return *this;
            }
        
            ObjectIDBuilder& Generate() {
                value = GenerateNextID();
                generated = true;
                return *this;
            }
        
            ObjectID Build() const {
                return ObjectID(value);
            }
        
        private:
            int GenerateNextID() {
                static std::atomic<int> nextId{1};
                return nextId.fetch_add(1);
            }
        
            int value = 0;
            bool generated = false;
    };

    class ObjectIDManager {
        public:

            void DestroyID(const ObjectID& id) {
                availableIDs.insert(id.GetAsInt());
                ObjectIDMap.erase(id);
            }
            
            ObjectID GenerateNewID() {
                if (!availableIDs.empty()) {
                    int id = *availableIDs.begin();
                    availableIDs.erase(availableIDs.begin());
                    return ObjectID(id);
                }
                return ObjectIDBuilder().Generate().Build();
            }

            void AssignID(ObjectID id, std::shared_ptr<Objects::Object> obj){
                ObjectIDMap[id] = obj;
            }
        
            std::shared_ptr<Objects::Object> GetObjectFromID(ObjectID id){
                auto it = ObjectIDMap.find(id);
                if (it != ObjectIDMap.end()) {
                    return it->second;
                }
                return nullptr;
            }

        private:
            std::map<ObjectID, std::shared_ptr<Objects::Object>> ObjectIDMap;
            std::unordered_set<int> availableIDs;
    };
}

namespace std {
    template<>
    struct hash<Epoch::Engine::ECS::ObjectID> {
        std::size_t operator()(const Epoch::Engine::ECS::ObjectID& id) const noexcept {
            return std::hash<int>{}(id.GetAsInt());
        }
    };
}