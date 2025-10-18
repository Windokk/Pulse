#include "assetID.hpp"

#include "engine/filesystem/filesystem.hpp"

namespace Epoch::Engine::Filesystem{
    
    void AssetIDManager::DestroyID(const AssetID &id)
    {
        availableIDs.insert(id.GetAsInt());
        AssetIDMap.erase(id);
    }

    AssetID AssetIDManager::GenerateNewID()
    {
        if (!availableIDs.empty()) {
            int id = *availableIDs.begin();
            availableIDs.erase(availableIDs.begin());
            return AssetID(id);
        }
        return AssetID(AssetIDBuilder().Generate().Build().GetAsInt());
    }

    void AssetIDManager::AssignID(AssetID id, std::shared_ptr<AssetInfo> info)
    {
        AssetIDMap[id] = info;
    }

    std::shared_ptr<AssetInfo> AssetIDManager::GetAssetFromID(AssetID id)
    {
        auto it = AssetIDMap.find(id);
        if (it != AssetIDMap.end()) {
            return it->second;
        }
        return nullptr;
    }

    AssetID AssetIDManager::GetIDFromRelativeFilePath(const Filesystem::Path &path)
    {
        for (const auto& [id, info] : AssetIDMap)
        {
            if(info->baseInfos.nameInProject == path.full){
                return id;
            }
        }

        return AssetID{};
    }
}
