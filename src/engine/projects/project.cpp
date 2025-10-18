#include "project.hpp"


namespace Epoch::Engine::Projects{
    Project::Project(std::string name, Filesystem::Path projectRoot, Filesystem::Path projectResourcesRoot, Filesystem::Path pluginsFolder, BuildSettings buildSettings, EditorPreferences editorPreferences, Filesystem::Path assetDatabasePath)
    {
        this->name = name;
        this->projectRoot = projectRoot;
        this->projectResourcesRoot = projectResourcesRoot;
        this->pluginsFolder = pluginsFolder;
        this->buildSettings = buildSettings;
        this->editorPreferences = editorPreferences;
        this->assetDatabasePath = assetDatabasePath;
    }
}


