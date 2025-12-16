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

    using ScalarValue = std::variant<bool, float, int, glm::vec2, glm::vec3, glm::vec4, glm::mat4>;

    using TextureValue = std::variant<std::shared_ptr<Texture>, std::shared_ptr<Cubemap>>;

    struct ScalarParameter {
        GLint location;
        GLenum glType;
        ScalarValue value;
    };

    struct TextureParameter {
        GLint location;
        GLenum samplerType;
        TextureValue texture;
    };

    class Material{

        public:
            Material(std::shared_ptr<Shader> shader, bool recievesShadows, RenderMode mode);
            
            void SetScalarParameter(const std::string &name, const ScalarValue &value)
            {
                if(scalarParameters.find(name) != scalarParameters.end()){
                    scalarParameters[name] = {scalarParameters[name].location, scalarParameters[name].glType, value};
                }
            }

            void SetTextureParameter(const std::string &name, const TextureValue &value)
            {
                if(textureParameters.find(name) != textureParameters.end()){
                    textureParameters[name] = {textureParameters[name].location, textureParameters[name].samplerType, value};
                }
            }

            ScalarValue GetScalarParameter(std::string name);
            std::shared_ptr<Texture> GetTexture(std::string name);
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

            std::unordered_map<std::string, ScalarParameter> scalarParameters;
            std::unordered_map<std::string, TextureParameter> textureParameters;

    };

}