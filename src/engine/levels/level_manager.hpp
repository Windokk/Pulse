#pragma once

#include "level.hpp"

namespace Pulse::Engine::Levels{

    class LevelManager {
        public:
            
            /// @brief Load a level
            /// @param lvl The pointer to the level
            void LoadLevel(std::shared_ptr<Level> lvl);

            /// @brief Getter for a loaded level
            /// @param index The index of the level to retrieve
            /// @return A pointer to the level loaded at "index"
            Level* GetLevelAt(int index);

            /// @brief Unload a loaded level
            /// @param index The index of the level to unload
            void UnloadLevel(int index);
            
            /// @brief Unload all loaded levels
            void UnloadAllLevels() {
                for(int i = 0; i < levelBuffer.size(); i++){
                    levelBuffer[i]->Unload();
                }
                levelBuffer.clear();
            }
    
            /// @brief Destroys a level
            void DestroyLevel(std::shared_ptr<Level> level){
                level->Unload();
            }
        
            /// @brief Destroys all levels
            void DestroyAllLevels() {
                for(int i = 0; i < levelBuffer.size(); i++){
                    levelBuffer[i]->Unload();
                }
                levelBuffer.clear();
            }
            
            /// @brief Getter for the total number loaded level
            /// @return The length of the level buffer
            int GetLoadedLevelCount() {
                return levelBuffer.size();
            }

            void Tick() {
                for(auto& level : levelBuffer){
                    level->Tick();
                }
            }
        
        private:

            std::vector<std::shared_ptr<Level>> levelBuffer;  // Buffer to store levels
        };
}