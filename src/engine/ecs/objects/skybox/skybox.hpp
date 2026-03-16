#pragma once

#include "engine/ecs/objects/level_object.hpp"

namespace Pulse::Engine::Rendering{
    class Shader;
    class Material;
    class EnvironmentMap;
    class Cubemap;
}


namespace Pulse::Engine::ECS::Objects
{
    class Skybox : public LevelObject{

        public:

            Skybox(std::shared_ptr<Rendering::EnvironmentMap> envMap, std::shared_ptr<Rendering::Material> material);

            void SetEnvironmentMap(std::shared_ptr<Rendering::EnvironmentMap> envMap);

            void SetMaterial(std::shared_ptr<Rendering::Material> material);

            void Destroy() override;

            std::shared_ptr<Rendering::EnvironmentMap> GetEnvMap() const { return m_EnvMap; }
            std::shared_ptr<Rendering::Material> GetMaterial() const { return m_Material; }

        private:

            void CreateDrawCommands();

            std::shared_ptr<Rendering::EnvironmentMap> m_EnvMap;
            std::shared_ptr<Rendering::Material> m_Material;
    };
}