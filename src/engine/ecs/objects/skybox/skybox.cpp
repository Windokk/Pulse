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

    void Skybox::Bind(std::shared_ptr<Rendering::Material> mat)
    {
        OpenGL* gl = Core::GetEngine().GetGL();

        mat->shader->Activate();

        const int matTexturesCount = mat->GetTexturesCount();

        gl->ActiveTexture(GL_TEXTURE0 + matTexturesCount);
        gl->BindTexture(GL_TEXTURE_CUBE_MAP, envMap->irradianceMap->GetID());
        mat->shader->SetInt("ibl_irradianceMap", matTexturesCount);

        gl->ActiveTexture(GL_TEXTURE0 + matTexturesCount + 1);
        gl->BindTexture(GL_TEXTURE_CUBE_MAP, envMap->prefilterMap->GetID());
        mat->shader->SetInt("ibl_prefilteredEnvMap", matTexturesCount + 1);

        gl->ActiveTexture(GL_TEXTURE0 + matTexturesCount + 2);
        gl->BindTexture(GL_TEXTURE_2D, envMap->brdfLUT->GetID());
        mat->shader->SetInt("ibl_brdfLUT", matTexturesCount + 2);
    }

    void Skybox::BindEmpty(std::shared_ptr<Rendering::Material> mat)
    {
        OpenGL* gl = Core::GetEngine().GetGL();

        Rendering::Renderer* renderer = Core::GetEngine().GetRenderer();

        mat->shader->Activate();

        const int matTexturesCount = mat->GetTexturesCount();

        gl->ActiveTexture(GL_TEXTURE0 + matTexturesCount);
        gl->BindTexture(GL_TEXTURE_CUBE_MAP, renderer->defaultCubemap->GetID());
        mat->shader->SetInt("ibl_irradianceMap", matTexturesCount);

        gl->ActiveTexture(GL_TEXTURE0 + matTexturesCount + 1);
        gl->BindTexture(GL_TEXTURE_CUBE_MAP, renderer->defaultCubemap->GetID());
        mat->shader->SetInt("ibl_prefilteredEnvMap", matTexturesCount + 1);

        gl->ActiveTexture(GL_TEXTURE0 + matTexturesCount + 2);
        gl->BindTexture(GL_TEXTURE_2D, renderer->defaultTexture->GetID());
        mat->shader->SetInt("ibl_brdfLUT", matTexturesCount + 2);
    }

    void Skybox::Destroy()
    {
        LevelObject::Destroy();
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