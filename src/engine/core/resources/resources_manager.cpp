#include "resources_manager.hpp"

#include "engine/serialization/material/material_serializer.hpp"

#include "engine/serialization/level/level_serializer.hpp"

#include "engine/serialization/assets/asset_database_serializer.hpp"

#include "engine/core/engine.hpp"

namespace Epoch::Engine::Core::Resources{

    using namespace Filesystem;

    void ResourcesManager::ConstructGlobalFileIndex(const Filesystem::Path &projectResDir)
    {
        Filesystem::Path projectDataBasePath = Core::GetEngine().GetCurrentProject()->GetAssetDatabasePath();
        Filesystem::Path engineDataBasePath = Core::GetEngine().GetFileManager()->GetEngineResRoot() / "asset_database.json";

        Serialization::DeserializeAssetDataBase(Core::GetEngine().GetFileManager()->GetEngineResRoot(), engineDataBasePath);
        
        Serialization::DeserializeAssetDataBase(projectResDir, projectDataBasePath);
        
    }

    std::shared_ptr<Rendering::Mesh> ResourcesManager::LoadModel(const std::string &name, const Filesystem::Path &path)
    {
        ufbx_load_opts opts = { 0 }; // Optional, pass NULL for defaults
        ufbx_error error; // Optional, pass NULL if you don't care about errors
        const std::string filePath = path.full;
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

        std::shared_ptr<Rendering::Mesh> mesh = std::make_shared<Rendering::Mesh>(ufbx_mesh, scene->settings.unit_meters, scene->materials);
        meshes.emplace(name, mesh);

        ufbx_free_scene(scene);

        return mesh;

    }

    std::shared_ptr<Rendering::Texture> ResourcesManager::LoadTexture(const std::string &name, const Filesystem::Path &path)
    {
        std::shared_ptr<Rendering::Texture> texture = std::make_shared<Rendering::Texture>(path);
        textures.emplace(name, texture);
        return texture;
    }

    std::shared_ptr<Rendering::Shader> ResourcesManager::LoadShader(const std::string &name, const Filesystem::Path &vsPath, const Filesystem::Path &fsPath, const Filesystem::Path &gsPath)
    {
        std::shared_ptr<Rendering::Shader> shader = std::make_shared<Rendering::Shader>(vsPath, fsPath, gsPath);
        shaders.emplace(name, shader);
        return shader;
    }

    std::shared_ptr<Rendering::Material> ResourcesManager::LoadMaterial(const std::string &name, const Filesystem::Path &path)
    {   
        LoadDependencies(name);

        std::shared_ptr<Rendering::Material> mat = Serialization::DeserializeMaterial(path);
        if(!mat){
            DEBUG_ERROR("Error during material import.");
        }
        materials.emplace(name, mat);
        return mat;
    }

    void ResourcesManager::LoadDependencies(const std::string &assetName){
        Filesystem::AssetIDManager* assetManager = Core::GetEngine().GetAssetIDManager();

        std::shared_ptr<Filesystem::AssetInfo> assetInfos = assetManager->GetAssetFromID(assetManager->GetIDFromRelativeFilePath(assetName));

        if (!assetInfos) {
            DEBUG_ERROR("Asset info not found for: " + assetName);
            return;
        }

        for(int i = 0; i < assetInfos->dependencies.size(); i++){
            LoadDependencies(assetManager->GetAssetFromID(assetInfos->dependencies[i])->baseInfos.nameInProject);
        }
    }

    std::shared_ptr<Levels::Level> ResourcesManager::LoadLevel(const std::string &name, const Filesystem::Path& path){

        LoadDependencies(name);

        std::shared_ptr<Levels::Level> level = Serialization::DeserializeLevel(path);
        if(!level){
            DEBUG_ERROR("Unknown error during level import.");
        }
        auto [it, inserted] = levels.emplace(name, level);
        if (!inserted) {
            DEBUG_WARNING("Level '" + name + "' already loaded.");
        }
        return level;
    }

    std::shared_ptr<Rendering::Mesh> ResourcesManager::GetMesh(std::string name)
    {
        auto it = meshes.find(name);
        if (it != meshes.end())
            return it->second;
        else{
            Filesystem::AssetIDManager* assetManager = Core::GetEngine().GetAssetIDManager();
            std::shared_ptr<Filesystem::AssetInfo> assetInfos = assetManager->GetAssetFromID(assetManager->GetIDFromRelativeFilePath(name));
            return LoadModel(name, assetInfos->baseInfos.path);
        }
    }

    std::shared_ptr<Rendering::Material> ResourcesManager::GetMaterial(std::string name)
    {
        auto it = materials.find(name);
        if (it != materials.end())
            return it->second;
        else{
            Filesystem::AssetIDManager* assetManager = Core::GetEngine().GetAssetIDManager();
            std::shared_ptr<Filesystem::AssetInfo> assetInfos = assetManager->GetAssetFromID(assetManager->GetIDFromRelativeFilePath(name));
            return LoadMaterial(name, assetInfos->baseInfos.path);
        }
    }

    std::shared_ptr<Rendering::Shader> ResourcesManager::GetShader(std::string name)
    {
        auto it = shaders.find(name);
        if (it != shaders.end()){
            return it->second;
        }
        else{
            Filesystem::AssetIDManager* assetManager = Core::GetEngine().GetAssetIDManager();
            std::shared_ptr<Filesystem::AssetInfo> assetInfos = assetManager->GetAssetFromID(assetManager->GetIDFromRelativeFilePath(name+".vert"));
            
            if(assetInfos == nullptr) return nullptr;

            Filesystem::Path vertPath = Filesystem::Path(assetInfos->baseInfos.path.GetParent()) / (assetInfos->baseInfos.name + ".vert");
            Filesystem::Path fragPath = Filesystem::Path(assetInfos->baseInfos.path.GetParent()) / (assetInfos->baseInfos.name + ".frag");
            Filesystem::Path geomPath = Filesystem::Path(assetInfos->baseInfos.path.GetParent()) / (assetInfos->baseInfos.name + ".geom");
            return LoadShader(name, vertPath, fragPath, geomPath.Exists() ? geomPath : Filesystem::Path(""));
        }
    }

    std::shared_ptr<Rendering::Texture> ResourcesManager::GetTexture(std::string name)
    {
        auto it = textures.find(name);
        if (it != textures.end())
            return it->second;
        else{
            Filesystem::AssetIDManager* assetManager = Core::GetEngine().GetAssetIDManager();
            std::shared_ptr<Filesystem::AssetInfo> assetInfos = assetManager->GetAssetFromID(assetManager->GetIDFromRelativeFilePath(name));
            return LoadTexture(name, assetInfos->baseInfos.path);
        }
    }

    std::shared_ptr<Levels::Level> ResourcesManager::GetLevel(const std::string &name)
    {
        if (auto it = levels.find(name); it != levels.end())
            return it->second;
        else{
            Filesystem::AssetIDManager* assetManager = Core::GetEngine().GetAssetIDManager();
            std::shared_ptr<Filesystem::AssetInfo> assetInfos = assetManager->GetAssetFromID(assetManager->GetIDFromRelativeFilePath(name));
            return LoadLevel(name, assetInfos->baseInfos.path);
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

    void ResourcesManager::UnloadTexture(const std::string &name)
    {
        auto it = textures.find(name);
        if (it != textures.end())
        {
            textures.erase(it);
        }
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