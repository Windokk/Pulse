#include "material.hpp"

#include <iostream>

#include "engine/debugging/logger.hpp"

#include "engine/filesystem/filesystem.hpp"

namespace Pulse::Engine::Rendering{

    UniformValue DefaultValueForType(UniformInfo uniform) {
        switch (uniform.type) {
            case GL_FLOAT: return 0.0f;
            case GL_INT: return 0;
            case GL_BOOL: return false;
            case GL_FLOAT_VEC3: return glm::vec3(0.0f);
            case GL_FLOAT_VEC4: return glm::vec4(0.0f);
            case GL_FLOAT_MAT4: return glm::mat4(1.0f);
            case GL_SAMPLER_2D: return std::shared_ptr<Texture>(nullptr);
            case GL_SAMPLER_2D_SHADOW: return std::shared_ptr<Texture>(nullptr);
            case GL_SAMPLER_CUBE: return 0;
            case GL_SAMPLER_CUBE_SHADOW: return 0;
            case GL_SAMPLER_CUBE_MAP_ARRAY: return 0;
            case GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW: return 0;
            default:{
                DEBUG_ERROR("Failed to query default variable type : "+std::to_string(uniform.type)+" , uniform name : "+uniform.name);
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
        this->renderMode = mode;
        this->recievesShadows = recievesShadows;
        this->shader = shader;

        auto uniforms = this->shader->GetActiveUniforms();
        for (const auto& uniform : uniforms) {
            parameters[uniform.name] = DefaultValueForType(uniform);
        }
    }

    std::shared_ptr<Texture> Material::GetTexture(std::string name)
    {
        for (const auto& [_name, _value] : parameters)
        {
            if (std::holds_alternative<std::shared_ptr<Texture>>(_value) && _name == name)
            {
                std::shared_ptr<Texture> val = std::get<std::shared_ptr<Texture>>(_value);
                if(val)
                    return val;
            }
        }

        DEBUG_WARNING("Couldn't find texture with name : "+name);

        return nullptr;
    }

    void Material::Use()
    {
        shader->Activate();

        int textureUnit = 0;

        for (const auto& [name, value] : parameters)
        {
            if (std::holds_alternative<float>(value)){
                shader->setFloat(name, std::get<float>(value));
            }
            else if (std::holds_alternative<int>(value)){
                shader->setInt(name, std::get<int>(value));
            }
            else if (std::holds_alternative<glm::vec2>(value)){
                shader->setVec2(name, std::get<glm::vec2>(value));
            }
            else if (std::holds_alternative<glm::vec3>(value)){
                shader->setVec3(name, std::get<glm::vec3>(value));
            }
            else if (std::holds_alternative<glm::vec4>(value)){
                shader->setVec4(name, std::get<glm::vec4>(value));
            }
            else if (std::holds_alternative<glm::mat4>(value)){
                shader->setMat4(name, std::get<glm::mat4>(value));
            }
            else if (std::holds_alternative<std::shared_ptr<Texture>>(value)){
                std::shared_ptr<Texture> val = std::get<std::shared_ptr<Texture>>(value);
                if (val != nullptr) {
                    val->Bind(textureUnit); // Bind to GL_TEXTURE0 + textureUnit
                    shader->setInt(name, textureUnit);
                    textureUnit++;
                }
            }
        }
    }

    void Material::StopUsing()
    {
        int textureUnit = 0;

        for (const auto& [name, value] : parameters)
        {
            std::visit([&](auto&& val) {
                using T = std::decay_t<decltype(val)>;

                if constexpr (std::is_same_v<T, std::shared_ptr<Texture>>)
                {
                    if (val) {
                        val->UnBind(textureUnit);
                        textureUnit++;
                    }
                }
            }, value);
        }

        shader->Deactivate();
    }
}
