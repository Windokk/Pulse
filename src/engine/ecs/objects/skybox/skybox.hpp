#pragma once

#include "engine/ecs/objects/object.hpp"

#include "engine/rendering/texture/cubemap.hpp"

namespace Pulse::Engine::Rendering{
    class Shader;

    class Material;
}


namespace Pulse::Engine::ECS::Objects
{
    class Skybox : public Object{

            public:

                Skybox(std::shared_ptr<Rendering::EnvironmentMap> envMap, std::shared_ptr<Rendering::Shader> shader);

                void SetCubemap(std::shared_ptr<Rendering::Cubemap> cubemap);

                void SetShader(std::shared_ptr<Rendering::Shader> shader);

                void Draw(glm::mat4 view, glm::mat4 projection);

                void Bind(std::shared_ptr<Rendering::Material> mat);

                void BindEmpty(std::shared_ptr<Rendering::Material> mat);

                void Destroy() override;

                unsigned int GetIrradianceID();
                unsigned int GetPrefilterID();
                unsigned int GetBrdfLutID();

                std::shared_ptr<Rendering::EnvironmentMap> envMap;


            private:
                std::shared_ptr<Rendering::Shader> shader;

    };
}