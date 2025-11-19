#include "skybox.hpp"

#include "engine/core/engine.hpp"

namespace Pulse::Engine::ECS::Objects{
    
    Skybox::Skybox(std::shared_ptr<Rendering::Cubemap> cubemap, std::shared_ptr<Rendering::Shader> shader)
    {
        this->cubemap = cubemap;
        this->shader = shader;
    }

    void Skybox::SetCubemap(std::shared_ptr<Rendering::Cubemap> cubemap)
    {
        this->cubemap = cubemap;
    }

    void Skybox::SetShader(std::shared_ptr<Rendering::Shader> shader)
    {
        this->shader = shader;
    }

    void Skybox::Draw(glm::mat4 view, glm::mat4 projection){

        if(!shader || !cubemap)
            return;

        this->cubemap->Draw(shader, view, projection);
    }

    void Skybox::Destroy()
    {
        Object::Destroy();
    }

}