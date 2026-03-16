#pragma once

#include <string>

#include "engine/core/objectID.hpp"
#include "engine/ecs/components/misc/script.hpp"
#include "engine/ecs/components/rendering/model_component.hpp"
#include "engine/ecs/objects/skybox/skybox.hpp"
#include "engine/filesystem/filesystem.hpp"

namespace Pulse::Engine::ECS{

    namespace Objects{
        class Actor;
    }

    namespace Components{
        class Light;
        class Camera;
        class Model;
        class Script;
        class AudioSource;
    }
}

namespace Pulse::Engine::Rendering{
    class Renderer;
    class Mesh;
    class Texture2D;
}

namespace Pulse::Engine::Levels{

    class Level{

        std::unordered_map<Core::ObjectID, std::shared_ptr<ECS::Objects::Actor>> rootActors;
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

            void AddActor(std::shared_ptr<ECS::Objects::Actor> a);
            void RemoveActor(Core::ObjectID id);
            std::shared_ptr<ECS::Objects::Actor> GetActor(Core::ObjectID id, bool recursive = false);
            std::vector<Core::ObjectID> GetActorsID(bool recursive = false);
            std::unordered_map<Core::ObjectID, std::shared_ptr<ECS::Objects::Actor>> GetRootActors() { return rootActors; };

            const std::string& GetName() const;
            void SetName(const std::string& name);
            
            void RemoveComponent(const int idInLevel, const std::shared_ptr<ECS::Components::Component> compPtr);

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
            std::shared_ptr<ECS::Objects::Skybox> skybox;
            std::shared_ptr<Rendering::Texture2D> ibl_texture;

            // These are maps for fast lookup (key: id IN LEVEL, value: ptr to the comp)
            std::vector<std::shared_ptr<ECS::Components::Light>> lights;
            std::unordered_map<int, std::shared_ptr<ECS::Components::Light>> lightComps;
            std::unordered_map<int, std::shared_ptr<ECS::Components::Transform>> transforms;
            std::unordered_map<int, std::shared_ptr<ECS::Components::Model>> models;
            std::unordered_map<int, std::shared_ptr<ECS::Components::PhysicsBody>> physicsBodies;
            std::unordered_map<int, std::shared_ptr<ECS::Components::AudioSource>> audioSources;
            std::unordered_map<int, std::shared_ptr<ECS::Components::Camera>> cameras;
            std::unordered_map<int, std::shared_ptr<ECS::Components::Script>> scripts;
            std::unordered_map<int, std::pair<glm::mat4, Rendering::Mesh*>> meshes;
            
    };

}
