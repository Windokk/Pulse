#include "material.hpp"

#include <iostream>

#include "engine/debugging/logger.hpp"

#include "engine/filesystem/filesystem.hpp"

#include "engine/core/engine.hpp"

namespace Pulse::Engine::Rendering{

    ScalarValue DefaultScalarValueForType(UniformInfo uniform) {
        switch (uniform.glType) {
            case GL_FLOAT: return 0.0f;
            case GL_INT: return 0;
            case GL_BOOL: return false;
            case GL_FLOAT_VEC2: return glm::vec2(0.0f);
            case GL_FLOAT_VEC3: return glm::vec3(0.0f);
            case GL_FLOAT_VEC4: return glm::vec4(0.0f);
            case GL_FLOAT_MAT4: return glm::mat4(1.0f);
            default:{
                DEBUG_ERROR("Failed to query default variable type : "+std::to_string(uniform.glType)+" , uniform name : "+uniform.name);
                return -1;
            }
        }
    }

    Material::Material(std::shared_ptr<Shader> shader, bool recievesShadows, RenderMode mode)
    {
        this->Init(shader, recievesShadows, mode);
    }
 
    void Material::Init(std::shared_ptr<Shader> shader, bool recievesShadows, RenderMode mode)
    {
        if(!shader)
            DEBUG_FATAL("Tried creating a material with a null shader");

        this->renderMode = mode;
        this->recievesShadows = recievesShadows;
        this->shader = shader;

        auto uniforms = this->shader->GetActiveUniforms();
        for (const auto& uniform : uniforms) {
            if (uniform.arraySize > 1) {
                std::string base = uniform.name;
                size_t pos = base.find("[0]");
                if (pos != std::string::npos) {
                    for (int i = 0; i < uniform.arraySize; i++) {
                        std::string elementName = base;
                        elementName.replace(pos, 3, "[" + std::to_string(i) + "]");
                        if(uniform.IsTexture()){ 
                            textureParameters[uniform.name] = {uniform.location, uniform.glType, 0};
                        }
                        else{
                            scalarParameters[uniform.name] = {uniform.location,uniform.glType, DefaultScalarValueForType(uniform)};
                        }
                    }
                    continue;
                }
            }

            if(uniform.IsTexture()){ 
                textureParameters[uniform.name] = {uniform.location, uniform.glType, 0};
            }
            else{
                scalarParameters[uniform.name] = {uniform.location,uniform.glType, DefaultScalarValueForType(uniform)};
            }
        }
    }

    ScalarValue Material::GetScalarParameter(std::string name)
    {
        for (const auto& [_name, _value] : scalarParameters)
        {
            if (_name == name)
            {
                return _value.value;
            }
        }

        DEBUG_WARNING("Couldn't find scalar with name : "+name);

        return 0;
    }

    void Material::Use()
    {
        shader->Activate();

        int textureUnit = 0;

        for (const auto& [name, scalar] : scalarParameters)
        {
            if (std::holds_alternative<float>(scalar.value)){
                shader->setFloat(name, std::get<float>(scalar.value));
            }
            else if (std::holds_alternative<int>(scalar.value)){
                shader->setInt(name, std::get<int>(scalar.value));
            }
            else if (std::holds_alternative<glm::vec2>(scalar.value)){
                shader->setVec2(name, std::get<glm::vec2>(scalar.value));
            }
            else if (std::holds_alternative<glm::vec3>(scalar.value)){
                shader->setVec3(name, std::get<glm::vec3>(scalar.value));
            }
            else if (std::holds_alternative<glm::vec4>(scalar.value)){
                shader->setVec4(name, std::get<glm::vec4>(scalar.value));
            }
            else if (std::holds_alternative<glm::mat4>(scalar.value)){
                shader->setMat4(name, std::get<glm::mat4>(scalar.value));
            }
        }

        for(const auto& [name, value] : textureParameters){
            Core::GetEngine().GetGL()->ActiveTexture(GL_TEXTURE0 + textureUnit);
            Core::GetEngine().GetGL()->BindTexture(value.samplerType, value.texture);
            shader->setInt(name, textureUnit);
            textureUnit++;
        }
    }

    void Material::StopUsing()
    {
        int textureUnit = 0;
        shader->Deactivate();
    }
}
