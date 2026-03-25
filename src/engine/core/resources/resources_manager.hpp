#pragma once

#include <unordered_map>

#include "engine/filesystem/filesystem.hpp"
#include "engine/rendering/mesh/mesh.hpp"
#include "engine/levels/level.hpp"

namespace Pulse::Engine::Rendering{
    class Renderer;
    class Mesh;
    class Texture2D;
    class Shader;
    class Material;
}

namespace Pulse::Engine::Core::Resources{

    class ResourcesManager{
        public:
            
            /// @brief Indexes all assets in the project and engine directories with unique IDs.
            /// Scans `projectDir` and `engineDir`, assigning each asset a unique ID via the Asset ID Manager.
            /// This enables consistent asset lookup and loading at runtime.
            /// @param projectResDir Path to the project's asset directory.
            void ConstructGlobalFileIndex(const Filesystem::Path &projectResDir);
            
            /// @brief Loads a FBX model from a path and adds it to the project's loaded models lists
            /// @param name The name of the model in the project
            /// @param path The normalized path at which the model is located (filename + extension expected)
            /// @return A shared pointer to a Mesh
            std::shared_ptr<Rendering::Mesh> LoadModel(const std::string &pathInProject, const Filesystem::Path &path);
            
            /// @brief Loads a texture2D from a given path and adds it to the project's loaded textures list
            /// @param name The name of the texture2D in the project
            /// @param path The normalized path at which the texture2D is located (filename + extension expected)
            /// @return A shared pointer to a Texture2D
            std::shared_ptr<Rendering::Texture2D> LoadTexture(const std::string &pathInProject, const Filesystem::Path &path);
            
            std::shared_ptr<Rendering::EnvironmentMap> LoadEnvMap(const std::string &pathInProject, const Filesystem::Path &path);

            /// @brief Loads a shader program from given vertex, fragment, and geometry shader paths and adds it to the project's loaded shaders list
            /// @param name The name of the shader program in the project
            /// @param vsPath The normalized path to the vertex shader source file
            /// @param fsPath The normalized path to the fragment shader source file
            /// @param gsPath The normalized path to the geometry shader source file (optional or empty if not used)
            /// @return A shared pointer to a Shader
            std::shared_ptr<Rendering::Shader> LoadShader(const std::string &pathInProject, const Filesystem::Path &vsPath, const Filesystem::Path &fsPath, const Filesystem::Path &gsPath);
            
            /// @brief Loads a material from a given path and adds it to the project's loaded materials list
            /// @param name The name of the material in the project
            /// @param path The normalized path at which the material is located (filename + extension expected)
            /// @return A shared pointer to a Material
            std::shared_ptr<Rendering::Material> LoadMaterial(const std::string &pathInProject, const Filesystem::Path &path);

            /// @brief Loads a level from a given path and adds it to the project's loaded levels list
            /// @param name The name of the level in the project
            /// @param path The normalized path at which the level is located (filename + extension expected)
            /// @return A shared pointer to a Level
            std::shared_ptr<Levels::Level> LoadLevel(const std::string &pathInProject, const Filesystem::Path& path);

            /// @brief Retrieves mesh from the project's loaded meshes lists (Tries to load it if it isn't loaded yet)
            /// @param name The name of the mesh in the project
            /// @return A shared pointer to a Mesh
            std::shared_ptr<Rendering::Mesh> GetMesh(std::string pathInProject);

            /// @brief Retrieves material from the project's loaded materials lists (Tries to load it if it isn't loaded yet)
            /// @param name The name of the material in the project
            /// @return A shared pointer to a Material
            std::shared_ptr<Rendering::Material> GetMaterial(std::string pathInProject);

            /// @brief Retrieves shader from the project's loaded shaders lists (Tries to load it if it isn't loaded yet)
            /// @param name The name of the shader in the project
            /// @return A shared pointer to a Shader
            std::shared_ptr<Rendering::Shader> GetShader(std::string pathInProject);

            /// @brief Retrieves a texture2D from the project's loaded textures lists (Tries to load it if it isn't loaded yet)
            /// @param name The name of the texture2D in the project
            /// @return A shared pointer to a Texture
            std::shared_ptr<Rendering::Texture2D> GetTexture(std::string pathInProject);

            std::shared_ptr<Rendering::EnvironmentMap> GetEnvMap(std::string pathInProject);

            /// @brief Retrieves level from the project's loaded levels lists (Tries to load it if it isn't loaded yet)
            /// @param name The name of the level in the project
            /// @return A shared pointer to a Level
            std::shared_ptr<Levels::Level> GetLevel(const std::string& pathInProject);

            /// @brief Unloads all unused dependencies of a given asset (recursively)
            /// @param assetName The name of the "root" asset
            void UnLoadDependencies(const std::string &assetName);

            void UnloadMesh(const std::string& name);
            void UnloadMaterial(const std::string& name);
            void UnloadShader(const std::string& name);
            void UnloadImage(const std::string& name);
            void UnloadEnvMap(const std::string& name);
            void UnloadLevel(const std::string& name);

        private:

            std::unordered_map<std::string, std::shared_ptr<Rendering::Mesh>> meshes;
            std::unordered_map<std::string, std::shared_ptr<Rendering::Texture2D>> textures;
            std::unordered_map<std::string, std::shared_ptr<Rendering::EnvironmentMap>> envmaps;
            std::unordered_map<std::string, std::shared_ptr<Rendering::Shader>> shaders;
            std::unordered_map<std::string, std::shared_ptr<Rendering::Material>> materials;
            std::unordered_map<std::string, std::shared_ptr<Levels::Level>> levels;
        };

}