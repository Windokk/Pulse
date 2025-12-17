#include "material.hpp"

#include <iostream>

#include "engine/debugging/logger.hpp"

#include "engine/filesystem/filesystem.hpp"

#include "engine/core/engine.hpp"

namespace Pulse::Engine::Rendering{

    NumericValue DefaultScalarValueForType(UniformInfo uniform) {
        switch (uniform.glType) {
            case GL_FLOAT: return 0.0f;
            case GL_INT: return 0;
            case GL_BOOL: return false;
            case GL_FLOAT_VEC2: return glm::vec2(0.0f);
            case GL_FLOAT_VEC3: return glm::vec3(0.0f);
            case GL_FLOAT_VEC4: return glm::vec4(0.0f);
            case GL_FLOAT_MAT4: return glm::mat4(1.0f);
            default:{
                DEBUG_ERROR("Failed to query default variable value for type : "+std::to_string(uniform.glType)+", uniform name : "+uniform.name);
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

        this->shader = shader;
        this->renderMode = mode;
        this->recievesShadows = recievesShadows;

        scalarParameters.clear();
        textureParameters.clear();

        auto uniforms = shader->GetActiveUniforms();

        for (const UniformInfo& uniform : uniforms)
        {
            // Skip uniforms optimized out by the compiler
            if (uniform.location == -1)
                continue;

            if(uniform.name.find("shadow_") == 0 || uniform.name.find("ibl_") == 0)
                continue;

            // arrays uniforms
            if (uniform.arraySize > 1)
            {
                std::string baseName = uniform.name;
                size_t pos = baseName.find("[0]");

                if (pos == std::string::npos)
                    continue;

                baseName = baseName.substr(0, pos);

                for (GLint i = 0; i < uniform.arraySize; ++i)
                {
                    std::string elementName = baseName + "[" + std::to_string(i) + "]";
                    GLint location = Core::GetEngine().GetGL()->GetUniformLocation(shader->ID, elementName.c_str());

                    if (location == -1)
                        continue;

                    if (uniform.IsTexture())
                    {
                        if(textureParameters.size() < 9){
                            textureParameters[elementName] = {
                                location,
                                uniform.glType,
                                0u
                            };
                        }
                    }
                    else
                    {
                        scalarParameters[elementName] = {
                            location,
                            uniform.glType,
                            DefaultScalarValueForType(uniform)
                        };
                    }
                }

                continue;
            }

            if (uniform.IsTexture())
            {
                if(textureParameters.size() < 9){
                    textureParameters[uniform.name] = {
                        uniform.location,
                        uniform.glType,
                        0u
                    };
                }
            }
            else
            {
                scalarParameters[uniform.name] = {
                    uniform.location,
                    uniform.glType,
                    DefaultScalarValueForType(uniform)
                };
            }
        }
    
        GLint unit = 0;

        for (auto& [name, tex] : textureParameters)
        {
            unit++;

            shader->Activate();
            shader->setIntLoc(tex.location, unit);
        }
        shader->Deactivate();
    }

    std::optional<TextureParameter> Material::GetTextureParameter(std::string name)
    {
        for (const auto& [_name, _value] : textureParameters)
        {
            if (_name == name)
            {
                return _value;
            }
        }

        return std::nullopt;
    }


    void Material::Use()
    {
        shader->Activate();

        for (const auto& [name, scalar] : scalarParameters)
        {
            if (std::holds_alternative<float>(scalar.value)){
                shader->setFloat(name, std::get<float>(scalar.value));
            }
            else if (std::holds_alternative<int>(scalar.value)){
                shader->setInt(name, std::get<int>(scalar.value));
            }
            else if (std::holds_alternative<bool>(scalar.value)){
                shader->setBool(name, std::get<bool>(scalar.value));
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

        int textureUnit = 0;
        const int maxUnits = Core::GetEngine().GetRenderer()->maxTextures;

        for (const auto& [name, value] : textureParameters)
        {
            if (value.texture == 0)
                continue;

            if (textureUnit >= maxUnits) {
                DEBUG_WARNING("Exceeded max texture units with texture : ", name);
                break;
            }

            Core::GetEngine().GetGL()->ActiveTexture(GL_TEXTURE0 + textureUnit);
            Core::GetEngine().GetGL()->BindTexture(value.samplerType, value.texture);

            // Tell shader which unit this sampler is on
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
