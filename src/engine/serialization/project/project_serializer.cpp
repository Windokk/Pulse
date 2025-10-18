#include "project_serializer.hpp"

#include "config.h"

#include <nlohmann/json.hpp>


#include "engine/debugging/debugger.hpp"

using namespace nlohmann;

namespace Epoch::Engine::Serialization{

    std::shared_ptr<Projects::Project> DeserializeProject(const Filesystem::Path path)
    {
        if(!path.Exists()){
            DEBUG_ERROR("Project at path : \"" + path.full +"\" doesn't exist !");
            return nullptr;
        }

        std::string src = path.ReadFile();

        try {
            json data = json::parse(src);

            int projectMajorVersion = -1;
            int projectMinorVersion = -1;
            int projectPatchVersion = -1;

            if(data.contains("versionMajor") && data["versionMajor"].is_number_integer()){
                projectMajorVersion = data["versionMajor"];
            }
            if(data.contains("versionMinor") && data["versionMinor"].is_number_integer()){
                projectMinorVersion = data["versionMinor"];
            }
            if(data.contains("versionPatch") && data["versionPatch"].is_number_integer()){
                projectPatchVersion = data["versionPatch"];
            }
            if(projectMajorVersion == -1 || projectMinorVersion == -1 || projectPatchVersion == -1){
                DEBUG_FATAL("No version specified/Incorrect version format for project: "+path.full);
                return nullptr;
            }
            else if(projectMajorVersion != PROJECT_VERSION_MAJOR || projectMinorVersion != PROJECT_VERSION_MINOR || projectPatchVersion != PROJECT_VERSION_PATCH){
                DEBUG_FATAL("Version incompatibility found between engine and project : "+path.full+ "\nProject version : "
                            + std::to_string(projectMajorVersion)+"."+std::to_string(projectMinorVersion)+"."+std::to_string(projectPatchVersion)+" Engine version : "+std::to_string(PROJECT_VERSION_MAJOR)+"."+std::to_string(PROJECT_VERSION_MINOR)+"."+std::to_string(PROJECT_VERSION_PATCH));
                return nullptr;
            }
            
            std::shared_ptr<Projects::Project> project = nullptr;

            Filesystem::Path projectResPath;
            Filesystem::Path projectRoot;
            Filesystem::Path pluginsPath;
            Filesystem::Path assetDatabasePath;

            if(data.contains("projectResources") && data["projectResources"].is_string()){
                projectResPath = Filesystem::Path(data["projectResources"]);
            }
            else{
                DEBUG_FATAL("No project resources specified for project: "+path.full);
            }

            if(data.contains("projectRoot") && data["projectRoot"].is_string()){
                projectRoot = Filesystem::Path(data["projectRoot"]);
            }
            else{
                DEBUG_FATAL("No project root specified for project: "+path.full);
            }

            if(data.contains("assetDatabase") && data["assetDatabase"].is_string()){
                assetDatabasePath = Filesystem::Path(data["assetDatabase"]);
            }
            else{
                DEBUG_FATAL("No asset database specified for project: "+path.full);
            }

            if(data.contains("pluginsFolder") && data["pluginsFolder"].is_string()){
                pluginsPath = Filesystem::Path(data["pluginsFolder"]);
            }
            else{
                DEBUG_WARNING("No plugins folder specified for project: "+path.full);
            }
            
            Projects::BuildSettings buildSettings;

            if(data.contains("buildSettings") && data["buildSettings"].is_array()){
                for(auto level : data["buildSettings"]){
                    if(!level.is_string())
                        continue;

                    buildSettings.AddToBuildSettings(Filesystem::Path(level));
                }
            }
            else{
                DEBUG_INFO("No build settings for project: "+path.full);
            }

            Projects::EditorPreferences editorPrefs = {};

            project = std::make_shared<Projects::Project>(path.GetFilename(false), projectRoot, projectResPath, pluginsPath, buildSettings, editorPrefs, assetDatabasePath);

            return project;

        } catch (const json::parse_error& e) {
            DEBUG_ERROR("JSON parse error: " + (std::string)e.what());
            return nullptr;
        }
    }

}