#include "resources_manager.hpp"

#include "engine/serialization/material/material_serializer.hpp"

#include "engine/serialization/assets/asset_database_serializer.hpp"

#include "engine/core/engine.hpp"

namespace Pulse::Engine::Core::Resources{

    using namespace Filesystem;

    void ResourcesManager::ConstructGlobalFileIndex(const Filesystem::Path &projectResDir)
    {
        Filesystem::Path projectDataBasePath = Core::GetEngine().GetCurrentProject()->GetAssetDatabasePath();
        Filesystem::Path engineDataBasePath = Core::GetEngine().GetFileManager()->GetEngineResRoot() / "asset_database.json";

        Serialization::DeserializeAssetDataBase(Core::GetEngine().GetFileManager()->GetEngineResRoot(), engineDataBasePath);
        
        Serialization::DeserializeAssetDataBase(projectResDir, projectDataBasePath);
        
    }

    std::shared_ptr<Rendering::Mesh> ResourcesManager::LoadModel(const std::string &pathInProject, const Filesystem::Path &path)
    {
        ufbx_load_opts opts = { 0 }; // Optional, pass NULL for defaults
        ufbx_error error; // Optional, pass NULL if you don't care about errors
        const std::string filePath = path.GetNativePath();
        ufbx_scene *scene = ufbx_load_file(filePath.c_str(), &opts, &error);
        if (!scene) {
            DEBUG_ERROR(
                "Failed to load " + path.full + " : " +
                (error.description.data ? error.description.data : "Unknown error"));
            return nullptr;
        }

        if (scene->meshes.count > 1) {
            ufbx_free_scene(scene);
            DEBUG_ERROR("Multiple meshes per fbx file isn't supported yet.");
            return nullptr;
        }

        ufbx_mesh* ufbx_mesh = scene->meshes.data[0];

        ufbx_node* mesh_node = nullptr;

        for (size_t i = 0; i < scene->nodes.count; i++) {
            ufbx_node* node = scene->nodes.data[i];
            if (node->mesh == ufbx_mesh) {
                mesh_node = node;
                break;
            }
        }

        std::shared_ptr<Rendering::Mesh> mesh = std::make_shared<Rendering::Mesh>(ufbx_mesh, scene->settings.unit_meters, scene->materials, mesh_node);
        meshes.emplace(pathInProject, mesh);
        mesh->SetAssetID(Core::GetEngine().GetAssetIDManager()->GetIDFromNameInProject(pathInProject));

        ufbx_free_scene(scene);

        return mesh;

    }

    std::shared_ptr<Rendering::Image> ResourcesManager::LoadImage(const std::string &pathInProject, const Filesystem::Path &path)
    {
        std::shared_ptr<Rendering::Image> img = std::make_shared<Rendering::Image>(path);
        images.emplace(pathInProject, img);
        img->SetAssetID(Core::GetEngine().GetAssetIDManager()->GetIDFromNameInProject(pathInProject));
        return img;
    }

    std::shared_ptr<Rendering::EnvironmentMap> ResourcesManager::LoadEnvMap(const std::string &pathInProject, const Filesystem::Path &path)
    {
        std::shared_ptr<Rendering::EnvironmentMap> envMap = std::make_shared<Rendering::EnvironmentMap>(path);
        if(!envMap){
            DEBUG_ERROR("Error during cubemap import");
            return nullptr;
        }
        envmaps.emplace(pathInProject, envMap);
        envMap->SetAssetID(Core::GetEngine().GetAssetIDManager()->GetIDFromNameInProject(pathInProject));
        return envMap;
    }

    std::shared_ptr<Rendering::Shader> ResourcesManager::LoadShader(const std::string &pathInProject, const Filesystem::Path &vsPath, const Filesystem::Path &fsPath, const Filesystem::Path &gsPath)
    {
        std::shared_ptr<Rendering::Shader> shader = std::make_shared<Rendering::Shader>(vsPath, fsPath, gsPath);
        shaders.emplace(pathInProject, shader);
        return shader;
    }

    std::shared_ptr<Rendering::Material> ResourcesManager::LoadMaterial(const std::string &pathInProject, const Filesystem::Path &path)
    {

        std::shared_ptr<Rendering::Material> mat = Serialization::DeserializeMaterial(path);
        if(!mat){
            DEBUG_ERROR("Error during material import.");
            return nullptr;
        }
        materials.emplace(pathInProject, mat);
        mat->SetAssetID(Core::GetEngine().GetAssetIDManager()->GetIDFromNameInProject(pathInProject));
        return mat;
    }

    std::shared_ptr<Levels::Level> ResourcesManager::LoadLevel(const std::string &pathInProject, const Filesystem::Path& path){

        std::shared_ptr<Levels::Level> level = std::make_shared<Levels::Level>("unitialized_level", path);
        int buildIndex = Core::GetEngine().GetBuildSettings()->GetLevelBuildIndex(pathInProject);
        if(buildIndex != -1){
            level->SetBuildIndex(buildIndex);
        }
        level->Deserialize(path);

        if(!level){
            DEBUG_ERROR("Unknown error during level import.");
            return nullptr;
        }
        auto [it, inserted] = levels.emplace(pathInProject, level);
        if (!inserted) {
            DEBUG_WARNING("Level '" + pathInProject + "' already loaded.");
        }
        level->SetAssetID(Core::GetEngine().GetAssetIDManager()->GetIDFromNameInProject(pathInProject));
        return level;
    }

    std::shared_ptr<Rendering::Mesh> ResourcesManager::GetMesh(std::string pathInProject)
    {
        auto it = meshes.find(pathInProject);
        if (it != meshes.end())
            return it->second;
        else{
            Filesystem::AssetIDManager* assetManager = Core::GetEngine().GetAssetIDManager();
            std::shared_ptr<Filesystem::AssetInfos> assetInfos = assetManager->GetAssetFromID(assetManager->GetIDFromNameInProject(pathInProject));
            
            if(assetInfos == nullptr){
                return nullptr;
            }

            return LoadModel(pathInProject, assetInfos->baseInfos.path);
        }
    }

    std::shared_ptr<Rendering::Material> ResourcesManager::GetMaterial(std::string pathInProject)
    {
        auto it = materials.find(pathInProject);
        if (it != materials.end())
            return it->second;
        else{
            Filesystem::AssetIDManager* assetManager = Core::GetEngine().GetAssetIDManager();
            std::shared_ptr<Filesystem::AssetInfos> assetInfos = assetManager->GetAssetFromID(assetManager->GetIDFromNameInProject(pathInProject));

            if(assetInfos == nullptr) return nullptr;

            return LoadMaterial(pathInProject, assetInfos->baseInfos.path);
        }
    }

    std::shared_ptr<Rendering::Shader> ResourcesManager::GetShader(std::string pathInProject)
    {
        auto it = shaders.find(pathInProject);
        if (it != shaders.end()){
            return it->second;
        }
        else{
            Filesystem::AssetIDManager* assetManager = Core::GetEngine().GetAssetIDManager();
            std::shared_ptr<Filesystem::AssetInfos> assetInfos = assetManager->GetAssetFromID(assetManager->GetIDFromNameInProject(pathInProject+".vert"));
            
            if(assetInfos == nullptr) return nullptr;

            Filesystem::Path vertPath = Filesystem::Path(assetInfos->baseInfos.path.GetParent()) / (assetInfos->baseInfos.name + ".vert");
            Filesystem::Path fragPath = Filesystem::Path(assetInfos->baseInfos.path.GetParent()) / (assetInfos->baseInfos.name + ".frag");
            Filesystem::Path geomPath = Filesystem::Path(assetInfos->baseInfos.path.GetParent()) / (assetInfos->baseInfos.name + ".geom");
            return LoadShader(pathInProject, vertPath, fragPath, geomPath.Exists() ? geomPath : Filesystem::Path(""));
        }
    }

    std::shared_ptr<Rendering::Image> ResourcesManager::GetImage(std::string pathInProject)
    {
        auto it = images.find(pathInProject);
        if (it != images.end())
            return it->second;
        else{
            Filesystem::AssetIDManager* assetManager = Core::GetEngine().GetAssetIDManager();
            std::shared_ptr<Filesystem::AssetInfos> assetInfos = assetManager->GetAssetFromID(assetManager->GetIDFromNameInProject(pathInProject));

            if(assetInfos == nullptr) return nullptr;

            return LoadImage(pathInProject, assetInfos->baseInfos.path);
        }
    }

    std::shared_ptr<Rendering::EnvironmentMap> ResourcesManager::GetEnvMap(std::string pathInProject)
    {
        auto it = envmaps.find(pathInProject);
        if (it != envmaps.end())
            return it->second;
        else{
            Filesystem::AssetIDManager* assetManager = Core::GetEngine().GetAssetIDManager();
            std::shared_ptr<Filesystem::AssetInfos> assetInfos = assetManager->GetAssetFromID(assetManager->GetIDFromNameInProject(pathInProject));

            if(assetInfos == nullptr) return nullptr;

            return LoadEnvMap(pathInProject, assetInfos->baseInfos.path);
        }
    }

    std::shared_ptr<Levels::Level> ResourcesManager::GetLevel(const std::string &pathInProject)
    {
        if (auto it = levels.find(pathInProject); it != levels.end())
            return it->second;
        else{
            Filesystem::AssetIDManager* assetManager = Core::GetEngine().GetAssetIDManager();
            std::shared_ptr<Filesystem::AssetInfos> assetInfos = assetManager->GetAssetFromID(assetManager->GetIDFromNameInProject(pathInProject));

            if(assetInfos == nullptr) return nullptr;

            return LoadLevel(pathInProject, assetInfos->baseInfos.path);
        }
    }

    void ResourcesManager::UnLoadDependencies(const std::string &assetName)
    {

    }

    void ResourcesManager::UnloadMesh(const std::string &name)
    {
        auto it = meshes.find(name);
        if (it != meshes.end())
        {
            meshes.erase(it);
        }
    }

    void ResourcesManager::UnloadMaterial(const std::string &name)
    {
        auto it = materials.find(name);
        if (it != materials.end())
        {
            materials.erase(it);
        }
    }

    void ResourcesManager::UnloadShader(const std::string &name)
    {
        auto it = shaders.find(name);
        if (it != shaders.end())
        {
            shaders.erase(it);
        }
    }

    void ResourcesManager::UnloadImage(const std::string &name)
    {
        auto it = images.find(name);
        if (it != images.end())
        {
            images.erase(it);
        }
    }

    void ResourcesManager::UnloadEnvMap(const std::string &name)
    {
    }

    void ResourcesManager::UnloadLevel(const std::string &name)
    {
        auto it = levels.find(name);
        if (it != levels.end())
        {
            levels.erase(it);
        }
    }
}