#include "asset_database_serializer.hpp"

#include "engine/debugging/debugger.hpp"
#include "engine/core/engine.hpp"

#include <nlohmann/json.hpp>

using namespace nlohmann;

namespace Epoch::Engine::Serialization{

    void DeserializeAssetDataBase(const Filesystem::Path resourcesPath, const Filesystem::Path databasePath){
        if(!databasePath.Exists()){
            DEBUG_ERROR("Database at path : \"" + databasePath.full +"\" doesn't exist !");
            return; 
        }

        if(!resourcesPath.Exists() || !resourcesPath.IsDirectory()){
            DEBUG_ERROR("Resources root at path : \"" + resourcesPath.full +"\" doesn't exist or is not a directory !");
            return;
        }

        std::string src = databasePath.ReadFile();

        try {
            json data = json::parse(src);

            for(auto [key, value] : data.items()){
                std::shared_ptr<Filesystem::AssetInfo> assetInfos = std::make_shared<Filesystem::AssetInfo>();
                std::vector<Filesystem::AssetID> dependencies;
                assetInfos->baseInfos = Core::GetEngine().GetFileManager()->GetFileInfos(resourcesPath / key);
                if (value.is_array() && value.size() > 1 && value[1].contains("dependencies")) {
                    for (const auto& dependencyID : value[1]["dependencies"]) {
                        if (dependencyID.is_number_integer()) {
                            dependencies.push_back(Filesystem::AssetID(dependencyID.get<int>()));
                        }
                    }
                }
                assetInfos->dependencies = dependencies;

                if (value.is_array() && value.size() >= 1 && value[0].contains("id")) {
                    Core::GetEngine().GetAssetIDManager()->AssignID(Filesystem::AssetIDBuilder().WithValue(value[0]["id"].get<int>()).Build(), assetInfos);
                }
            }
            

        } catch (const json::parse_error& e) {
            DEBUG_ERROR("JSON parse error: " + (std::string)e.what());
            return;
        }
    }

}