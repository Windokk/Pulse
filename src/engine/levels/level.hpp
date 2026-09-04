#pragma once

#include <string>
#include <vector>

#include "engine/core/objectID.hpp"
#include "engine/objects/components/misc/script.hpp"
#include "engine/objects/components/rendering/model_component.hpp"
#include "engine/objects/skybox/skybox.hpp"
#include "engine/filesystem/filesystem.hpp"

namespace Pulse::Engine::Objects{

    class Actor;

    namespace Components{
        class Light;
        class Camera;
        class Model;
        class Script;
        class AudioSource;
        class ProbeVolume;
    }
}

namespace Pulse::Engine::Rendering{
    class Renderer;
    class Mesh;
    class Texture2D;
}

namespace Pulse::Engine::Levels{

    struct LevelAssetManifest
    {
        bool success = false;
        std::vector<std::string> meshPathsInProject;
        std::vector<std::string> materialPathsInProject;
        std::string skyboxEnvMapPathInProject;
    };

    LevelAssetManifest CollectLevelAssetRefs(const Filesystem::Path& filePath);

    class Level{

        std::unordered_map<Core::ObjectID, std::shared_ptr<Objects::Actor>> rootActors;
        std::string name;
        
        Filesystem::Path path;

        bool loaded = false;

        int buildIndex = -1;

        Filesystem::AssetID assetID;

        public:
            Level(std::string name, Filesystem::Path path);

            void Deserialize(Filesystem::Path filePath);
            void Serialize(Filesystem::Path filePath);

            void SetBuildIndex(int buildIndex);

            void RemoveActorRecursive(Core::ObjectID actorID);

            void Clear();

            void Tick();
            void Play();
            void Stop();
            void OnLoad();
            void Unload();

            Filesystem::Path GetPath() { return path; }

            void AddActor(std::shared_ptr<Objects::Actor> a);
            void RemoveActor(Core::ObjectID id);
            std::shared_ptr<Objects::Actor> GetActor(Core::ObjectID id, bool recursive = false);
            std::vector<Core::ObjectID> GetActorsID(bool recursive = false);
            std::unordered_map<Core::ObjectID, std::shared_ptr<Objects::Actor>> GetRootActors() { return rootActors; };

            const std::string& GetName() const;
            void SetName(const std::string& name);
            
            void RemoveComponent(const int idInLevel, const std::shared_ptr<Objects::Components::Component> compPtr);

            void SetAssetID(Filesystem::AssetID assetID) {
                this->assetID = assetID;
            }

            Filesystem::AssetID GetAssetID() {
                return this->assetID;
            }

            int GetBuildIndex() { return buildIndex; }

            bool IsLoaded() { return loaded; }

            void SetLoaded(bool loaded) { this->loaded = loaded; }

            float ambientIntensity = 0.2f;
            std::shared_ptr<Objects::Skybox> skybox;
            std::shared_ptr<Rendering::Texture2D> ibl_texture;

            std::shared_ptr<Objects::Components::ProbeVolume> probeVolume;

            // These are maps for fast lookup (key: id IN LEVEL, value: ptr to the comp)
            std::vector<std::shared_ptr<Objects::Components::Light>> lights;
            std::unordered_map<int, std::shared_ptr<Objects::Components::Light>> lightComps;
            std::unordered_map<int, std::shared_ptr<Objects::Components::Transform>> transforms;
            std::unordered_map<int, std::shared_ptr<Objects::Components::Model>> models;
            std::unordered_map<int, std::shared_ptr<Objects::Components::PhysicsBody>> physicsBodies;
            std::unordered_map<int, std::shared_ptr<Objects::Components::AudioSource>> audioSources;
            std::unordered_map<int, std::shared_ptr<Objects::Components::Camera>> cameras;
            std::unordered_map<int, std::shared_ptr<Objects::Components::Script>> scripts;
            std::unordered_map<int, std::pair<glm::mat4, Rendering::Mesh*>> meshes;
            
    };

}
