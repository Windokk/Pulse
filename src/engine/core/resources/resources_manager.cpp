#include "resources_manager.hpp"

#include "engine/serialization/material/material_serializer.hpp"

#include "engine/serialization/level/level_serializer.hpp"

namespace Epoch::Engine::Core::Resources{

    using namespace Filesystem;

    void ResourcesManager::LoadResourcesInDir(const Filesystem::Path &baseDir){
        
        for(Filesystem::FileInfo infos : Filesystem::FileManager::ListDirectory(baseDir, {Type::T_IMAGE, Type::T_SHADER}, false, true)){
            switch(infos.path.GetExtensionType()){
                case Type::T_IMAGE:{
                    std::string textureName = infos.path.RelativeTo(baseDir).full;
                    if (textures.find(textureName) != textures.end()) {
                        break;
                    }
                    LoadTexture(textureName, infos.path);
                    DEBUG_LOG("Loaded texture : "+textureName);
                    break;
                }
                case Type::T_SHADER: {
                    if(infos.path.GetParentPath().IsDirectory()){

                        std::string shaderName = infos.path.RelativeTo(baseDir).WithoutExtension();

                        if (shaders.find(shaderName) != shaders.end()) {
                            break;
                        }

                        Filesystem::Path vertPath = Filesystem::Path(
                                                        (std::filesystem::path(infos.path.GetParent()) / (infos.name + ".vert")).string()
                                                    );
                        Filesystem::Path fragPath = Filesystem::Path(
                                                        (std::filesystem::path(infos.path.GetParent()) / (infos.name + ".frag")).string()
                                                    );
                        Filesystem::Path geomPath = Filesystem::Path(
                                                        (std::filesystem::path(infos.path.GetParent()) / (infos.name + ".geom")).string()
                                                    );

                        
                        if(vertPath.Exists() && fragPath.Exists()){
                            DEBUG_INFO("Loading shader : " + shaderName);
                            LoadShader(shaderName, vertPath, fragPath, geomPath.Exists() ? geomPath : Filesystem::Path(""));
                        }   
                        else{
                            DEBUG_ERROR("Shader loading failed. Missing .vert or .frag file in directory.");
                        }
                    }
                    break;
                }
            }
        }

        for(Filesystem::FileInfo infos : Filesystem::FileManager::ListDirectory(baseDir, {Type::T_MATERIAL, Type::T_MODEL}, false, true)){
            switch(infos.path.GetExtensionType()){
                case Type::T_MODEL:{
                    std::string meshName = infos.path.RelativeTo(baseDir).full;
                    if (meshes.find(meshName) != meshes.end()) {
                        break;
                    }
                    DEBUG_LOG("Loading model : "+meshName);
                    LoadModel(meshName, infos.path);
                    break;
                }
                case Type::T_MATERIAL:{
                    std::string matName = infos.path.RelativeTo(baseDir).WithoutExtension();
                    if (materials.find(matName) != materials.end()) {
                        break;
                    }
                    DEBUG_LOG("Loading material : "+matName);
                    LoadMaterial(matName, infos.path);
                    break;
                }
            }
        }
    
        for(Filesystem::FileInfo infos : Filesystem::FileManager::ListDirectory(baseDir, {Type::T_LEVEL}, false, true)){
            std::string levelName = infos.path.RelativeTo(baseDir).full;
            if (levels.find(levelName) != levels.end()) {
                break;
            }
            DEBUG_INFO("Loading level : " + levelName);
            LoadLevel(levelName, infos.path);
        }
    
    }

    void ResourcesManager::LoadResources(const Filesystem::Path &projectDir, const Filesystem::Path &engineDir)
    {
        DEBUG_LOG("Loading resources from base directory:"+ projectDir.full);

        if(!projectDir.Exists() || !projectDir.IsDirectory()){
            DEBUG_FATAL("Project directory for resources does not exist or is not a directory.");
        }

        if(!engineDir.Exists() || !engineDir.IsDirectory()){
            DEBUG_FATAL("Engine directory for resources does not exist or is not a directory.");
        }

        LoadResourcesInDir(engineDir);
        LoadResourcesInDir(projectDir);

        DEBUG_LOG("Loaded all resources correctly !");
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
        std::shared_ptr<Rendering::Material> mat = Serialization::MaterialSerializer::ImportMaterial(path);
        if(!mat){
            DEBUG_ERROR("Error during material import.");
        }
        materials.emplace(name, mat);
        return mat;
    }

    std::shared_ptr<Levels::Level> ResourcesManager::LoadLevel(const std::string &name, const Filesystem::Path& path){
        std::shared_ptr<Levels::Level> level = Serialization::ImportLevel(path);
        if(!level){
            DEBUG_ERROR("Unknown error during level import.");
        }
        auto [it, inserted] = levels.emplace(name, level);
        if (!inserted) {
            DEBUG_WARNING("Level '" + name + "' already loaded.");
        }
        return level;
    }
}