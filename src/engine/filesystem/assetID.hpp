#pragma once

#include <string>
#include <atomic>
#include <unordered_set>
#include <map>
#include <memory>

namespace Pulse::Engine::Filesystem
{
    struct AssetInfos;

    class Path;

    class AssetID {
        public:
            
            AssetID() : packed(0) {}
            explicit AssetID(int value) : packed(value) {}

            
            int GetAsInt() const {
                return packed;
            }

            std::string GetAsString() const {
                return std::to_string(GetAsInt());
            }

            
            bool operator==(const AssetID& other) const { return packed == other.packed; }
            bool operator!=(const AssetID& other) const { return !(*this == other); }
            bool operator<(const AssetID& other) const { return packed < other.packed; }

            
            friend class AssetIDBuilder;
    
        private:
            int packed;
    };
    
    class AssetIDBuilder {
        public:
            AssetIDBuilder& WithValue(int val) {
                value = val;
                generated = false;
                return *this;
            }
        
            AssetIDBuilder& Generate() {
                value = GenerateNextID();
                generated = true;
                return *this;
            }
        
            AssetID Build() const {
                return AssetID(value);
            }
        
        private:
            static int GenerateNextID() {
                static std::atomic<int> nextId{1};
                return nextId.fetch_add(1);
            }
        
            int value = 0;
            bool generated = false;
    };

    struct FileInfos; 

    class AssetIDManager {
        public:

            void DestroyID(const AssetID& id);
            
            AssetID GenerateNewID();

            void AssignID(AssetID id, std::shared_ptr<AssetInfos> info);
        
            std::shared_ptr<AssetInfos> GetAssetFromID(AssetID id);

            AssetID GetIDFromNameInProject(const std::string nameInProject);

            std::map<AssetID, std::shared_ptr<AssetInfos>> AssetIDMap;
        private:
            std::unordered_set<int> availableIDs;
    };
}
