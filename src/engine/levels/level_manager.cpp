#include "engine/levels/level_manager.hpp"

#include "engine/debugging/logger.hpp"
#include "level_manager.hpp"

#include "engine/core/engine.hpp"
#include "engine/core/resources/resources_manager.hpp"
#include "engine/core/objectID.hpp"
#include "engine/rendering/renderer/renderer.hpp"

#include "engine/projects/project.hpp"

namespace Pulse::Engine::Levels{
    void LevelManager::LoadLevel(std::shared_ptr<Level> lvl)
    {
        if(!lvl)
        DEBUG_FATAL("Cannot load level (because pointer is null)");
        levelBuffer.push_back(lvl);
        lvl->SetLoaded(true);

        int levelBuildIndex = lvl->GetBuildIndex();
        int levelAssetID = Core::GetEngine().GetAssetIDManager()->GetIDFromNameInProject(Core::GetEngine().GetBuildSettings()->buildIndex[levelBuildIndex].full).GetAsInt();

        Core::GetEngine().GetEventDispatcher()->emitGlobal(Events::LevelStructureChangedEvent(
                                                    levelAssetID, Events::LOADED, "", Core::ObjectID(-1)));

        lvl->OnLoad();
    }

    void LevelManager::LoadLevelAsync(const std::string &pathInProject)
    {
        if(asyncLoadPending){
            DEBUG_WARNING("Level load already in progress, ignoring request to load : " + pathInProject);
            return;
        }

        asyncLoadPathInProject = pathInProject;
        asyncLoadPending = true;
        prefetcher.BeginLoad(pathInProject);
    }

    void LevelManager::LoadLevelBlocking(const std::string &pathInProject, const std::function<void(float)> &tickCallback)
    {
        if(asyncLoadPending){
            DEBUG_WARNING("Level load already in progress, ignoring request to load : " + pathInProject);
            return;
        }

        asyncLoadPathInProject = pathInProject;
        asyncLoadPending = true;
        prefetcher.BeginLoad(pathInProject);

        while(!prefetcher.Pump()){
            if(tickCallback)
                tickCallback(prefetcher.GetProgress());
        }

        asyncLoadPending = false;
        FinishAsyncLoad();
    }

    void LevelManager::PumpAsyncLoad()
    {
        if(!asyncLoadPending)
            return;

        if(!prefetcher.Pump())
            return;

        asyncLoadPending = false;
        FinishAsyncLoad();
    }

    void LevelManager::FinishAsyncLoad()
    {
        std::string pathInProject = asyncLoadPathInProject;

        auto& engine = Core::GetEngine();
        auto level = engine.GetResourcesManager()->GetLevel(pathInProject);

        if(!level){
            DEBUG_ERROR("Error loading level : " + pathInProject);
            return;
        }

        if(!levelBuffer.empty()){
            auto* resourcesManager = engine.GetResourcesManager();
            auto* assetIDManager = engine.GetAssetIDManager();

            while(!levelBuffer.empty()){
                std::string nameInProject = assetIDManager->GetAssetFromID(levelBuffer[0]->GetAssetID())->baseInfos.nameInProject;
                UnloadLevel(0);
                resourcesManager->UnloadLevel(nameInProject);
            }

            engine.GetRenderer()->ClearPassesContent();
            engine.GetObjectIDManager()->Reset();
        }

        LoadLevel(level);
    }

    Level* LevelManager::GetLevelAt(int index){
        if (index >= 0 && index < levelBuffer.size()) {
            return levelBuffer[index].get();
        } else {
            DEBUG_ERROR("Invalid index (out of bounds). Unable to retrieve level.");
            return nullptr;
        }
    }

    void LevelManager::UnloadLevel(int index){
        if (index >= 0 && index < levelBuffer.size()) {
            levelBuffer[index]->Unload();
            levelBuffer.erase(levelBuffer.begin() + index);
        } else {
            DEBUG_ERROR("Invalid index (out of bounds). Unable to unload level.");
        }
    }
}