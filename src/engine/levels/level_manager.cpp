#include "engine/levels/level_manager.hpp"

#include "engine/debugging/debugger.hpp"
#include "level_manager.hpp"

namespace Epoch::Engine::Levels{
    void LevelManager::LoadLevel(std::shared_ptr<Level> lvl)
    {
        if(!lvl)
        DEBUG_FATAL("Cannot load level (because pointer is null)");
        levelBuffer.push_back(lvl);
        lvl->loaded = true;
        lvl->Start();
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