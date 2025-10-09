#pragma once

#include <string>

#include "engine/rendering/renderer/renderer.hpp"

#include "engine/ecs/components/physics/physics_body.hpp"
#include "engine/ecs/components/audio/audio_source.hpp"
#include "engine/ecs/components/rendering/camera.hpp"
#include "engine/ecs/components/core/script.hpp"

namespace Epoch::Engine::ECS{

    namespace Objects{
        class Actor;
    }

    class ObjectID;

    namespace Components{
        class Light;
    }
}

namespace Epoch::Engine::Rendering{
    class Renderer;
    class Mesh;
}

namespace Epoch::Engine::Levels{

    class Level{

        std::vector<std::shared_ptr<ECS::Objects::Actor>> rootActors;
        std::string name;
        std::string path;

        public:
        Level(std::string name);

        void Clear();

        void Tick();
        void Start();
        void Unload();

        void AddActor(std::shared_ptr<ECS::Objects::Actor> a);
        void RemoveActor(ECS::ObjectID id, bool recursive = false);
        std::shared_ptr<ECS::Objects::Actor> GetActor(ECS::ObjectID id, bool recursive = false);
        std::vector<ECS::ObjectID> GetActorsID(bool recursive = false);
        std::vector<std::shared_ptr<ECS::Objects::Actor>> GetRootActors() { return rootActors; };

        const std::string& GetName() const;
        void SetName(const std::string& name);

        std::vector<std::shared_ptr<ECS::Components::Light>> lights;
        std::vector<std::shared_ptr<ECS::Components::Transform>> transforms;
        std::vector<std::shared_ptr<ECS::Components::Model>> models;
        std::vector<std::shared_ptr<ECS::Components::PhysicsBody>> physicsBodies;
        std::vector<std::shared_ptr<ECS::Components::AudioSource>> audioSources;
        std::vector<std::shared_ptr<ECS::Components::Camera>> cameras;
        std::vector<std::shared_ptr<ECS::Components::Script>> scripts;

        /// @brief 
        std::unordered_map<int, std::pair<glm::mat4, Rendering::Mesh*>> meshes;
        
        bool loaded = false;
        
    };

}
