#include "skybox.hpp"

#include "engine/core/engine.hpp"

namespace Pulse::Engine::ECS::Objects{
    
    Skybox::Skybox(std::shared_ptr<Rendering::EnvironmentMap> envMap, std::shared_ptr<Rendering::Shader> shader)
    {
        this->envMap = envMap;
        this->shader = shader;
    }

    void Skybox::SetCubemap(std::shared_ptr<Rendering::Cubemap> cubemap)
    {
        this->envMap = envMap;
    }

    void Skybox::SetShader(std::shared_ptr<Rendering::Shader> shader)
    {
        this->shader = shader;
    }

    void Skybox::Draw(glm::mat4 view, glm::mat4 projection){

        if(!shader || !envMap)
            return;

        this->envMap->Draw(shader, view, projection);
    }

    void Skybox::Destroy()
    {
        Object::Destroy();
    }

    unsigned int Skybox::GetIrradianceID()
    {
        if(envMap) 
            return envMap->irradianceMap->GetID(); 
        return 0;
    }

    unsigned int Skybox::GetPrefilterID()
    {
        if(envMap) 
            return envMap->prefilterMap->GetID(); 
        return 0;
    }

    unsigned int Skybox::GetBrdfLutID()
    {
        if(envMap) 
            return envMap->brdfLUT->GetID(); 
        return 0;
    }

}