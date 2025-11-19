#pragma once

#include "engine/ecs/objects/object.hpp"

#include "engine/rendering/texture/cubemap.hpp"

namespace Pulse::Engine::Rendering{
    class Shader;
}


namespace Pulse::Engine::ECS::Objects
{
    class Skybox : public Object{

            public:

                Skybox(std::shared_ptr<Rendering::Cubemap> cubemap, std::shared_ptr<Rendering::Shader> shader);

                void SetCubemap(std::shared_ptr<Rendering::Cubemap> cubemap);

                void SetShader(std::shared_ptr<Rendering::Shader> shader);

                void Draw(glm::mat4 view, glm::mat4 projection);

                void Destroy() override;

                unsigned int GetIrradianceID() { 
                    if(cubemap) 
                        return cubemap->GetIrradianceID(); 
                    return 0;
                }
                unsigned int GetPrefilterID() { 
                    if(cubemap) 
                        return cubemap->GetPrefilterID(); 
                    return 0;
                }
                unsigned int GetBrdfLutID() { 
                    if(cubemap) 
                        return cubemap->GetBrdfLutID(); 
                    return 0;
                }


            private:
                std::shared_ptr<Rendering::Shader> shader;
                std::shared_ptr<Rendering::Cubemap> cubemap;

    };
}