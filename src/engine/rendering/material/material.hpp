#pragma once

#include "engine/rendering/shader/shader.hpp"
#include "engine/rendering/texture/texture.hpp"
#include "engine/rendering/texture/cubemap.hpp"

#include <variant>
#include <stdexcept>
#include <memory>

#include "engine/debugging/logger.hpp"

namespace Pulse::Engine::Rendering {

    enum RenderMode{
        OPAQUE,
        TRANSLUCENT,
        MASKED
    };

    using NumericValue = std::variant<bool, float, int, glm::vec2, glm::vec3, glm::vec4, glm::mat4>;

    struct NumericParameter {
        GLint location;
        GLenum glType;
        NumericValue value = -1;
    };

    struct TextureParameter {
        GLint location;
        GLenum samplerType;
        GLuint texture = 0;
    };

    class Material{

        public:
            Material(std::shared_ptr<Shader> shader, bool recievesShadows, RenderMode mode);
            
            void SetScalarParameter(const std::string &name, const NumericValue &value)
            {
                if(scalarParameters.find(name) != scalarParameters.end()){
                    scalarParameters[name] = {scalarParameters[name].location, scalarParameters[name].glType, value};
                }
            }

            void SetTextureParameter(const std::string &name, const GLuint &value)
            {
                if(textureParameters.find(name) != textureParameters.end()){
                    textureParameters[name] = {textureParameters[name].location, textureParameters[name].samplerType, value};
                }
            }

            template<typename T>
            T GetScalarParameter(std::string name, T defaultValue = {});
            std::optional<TextureParameter> GetTextureParameter(std::string name);

            int GetTexturesCount(){
                return textureParameters.size();
            }

            void Use();
            void StopUsing();
            std::shared_ptr<Shader> shader;
            bool recievesShadows = false;
            RenderMode renderMode = OPAQUE;

            void SetAssetID(Filesystem::AssetID assetID) {
                this->assetID = assetID;
            }

            Filesystem::AssetID GetAssetID() {
                return this->assetID;
            }

        private:
            Filesystem::AssetID assetID;
            void Init(std::shared_ptr<Shader> shader, bool recievesShadows, RenderMode mode);

            std::unordered_map<std::string, NumericParameter> scalarParameters;
            std::unordered_map<std::string, TextureParameter> textureParameters;

    };


    template<typename T>
    T GetScalarParameter(const std::string& name, T defaultValue)
    {
        auto param = GetScalarParameter(name);
        if (!param)
            return defaultValue;

        if (auto v = std::get_if<T>(&param->value))
            return *v;

        return defaultValue;
    }

}