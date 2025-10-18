#pragma once

#include <string>

#include "engine/filesystem/filesystem.hpp"
#include "engine/debugging/debugger.hpp"

namespace Epoch::Engine::Projects{

    struct BuildSettings{
        // TODO : Target system


        /// @brief A list of level paths to include in the build
        std::vector<Filesystem::Path> buildIndex = {};
        

        int GetLevelBuildIndex(const Filesystem::Path lvlPath) {

            for(int i = 0; i < buildIndex.size(); i++){
                if(buildIndex[i] == lvlPath){
                    return i;
                }
            }
            return -1; // Not found
        }

        void AddToBuildSettings(Filesystem::Path lvlPath){
            
            for(int i = 0; i < buildIndex.size(); i++){
                if(buildIndex[i] == lvlPath){
                    DEBUG_INFO("Level already added to build settings, not re-adding it");
                    return;
                }
            }

            buildIndex.push_back(lvlPath);
        }

        void ChangeBuildIndex(Filesystem::Path lvlPath, int newBuildIndex) {
            int oldBuildIndex = GetLevelBuildIndex(lvlPath);
            if (oldBuildIndex == -1) {
                DEBUG_ERROR("Tried to change level's build index, but level is not yet registered in the build settings");
                return;
            }

            auto& indexList = buildIndex;

            // Erase the level from its old pos
            indexList.erase(indexList.begin() + oldBuildIndex);

            if (newBuildIndex > oldBuildIndex) {
                --newBuildIndex;
            }

            newBuildIndex = std::clamp(newBuildIndex, 0, static_cast<int>(indexList.size()));

            // Insert the level at the new position
            indexList.insert(indexList.begin() + newBuildIndex, lvlPath);
        }
    };

    struct EditorPreferences{
        // TODO
    };

    class Project{
        public:

            std::string name;

            Project(std::string name, Filesystem::Path projectRoot,
                    Filesystem::Path projectResourcesRoot,
                    Filesystem::Path pluginsFolder,
                    BuildSettings buildSettings,
                    EditorPreferences editorPreferences, Filesystem::Path assetDatabasePath);

            Filesystem::Path GetProjectResourcesPath() { return projectResourcesRoot; }
            Filesystem::Path GetProjectRoot() { return projectRoot; }
            Filesystem::Path GetAssetDatabasePath() { return assetDatabasePath; }
            Filesystem::Path GetPluginsFolderPath() { return pluginsFolder; }

            BuildSettings* GetBuildSettings() { return &buildSettings; }

            EditorPreferences GetEditorPrefs() { return editorPreferences; }
            
        private:
            Filesystem::Path projectRoot;
            Filesystem::Path projectResourcesRoot;
            Filesystem::Path pluginsFolder;
            Filesystem::Path assetDatabasePath;
            BuildSettings buildSettings;
            EditorPreferences editorPreferences;
    };

}