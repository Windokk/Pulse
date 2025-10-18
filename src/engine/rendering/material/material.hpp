#pragma once

#include "engine/rendering/shader/shader.hpp"
#include "engine/rendering/texture/texture.hpp"

#include <variant>
#include <stdexcept>
#include <memory>

namespace Epoch::Engine::Rendering {

    enum RenderMode{
        OPAQUE,
        TRANSLUCENT,
        MASKED
    };

    using UniformValue = std::variant<float, int, glm::vec2, glm::vec3, glm::vec4, glm::mat4, std::shared_ptr<Texture>>;

    class Material{

        public:
            Material(std::shared_ptr<Shader> shader, bool recievesShadows, RenderMode mode);
            
            void SetParameter(const std::string &name, const UniformValue &value)
            {
                parameters[name] = value;
            }
    
            std::map<std::string, UniformValue>* GetParameters() { return &parameters; };
            std::shared_ptr<Texture> GetTexture(std::string name);
            void Use();
            void StopUsing();
            std::shared_ptr<Shader> shader;
            bool recievesShadows = false;
            RenderMode renderMode = OPAQUE;
            
            Filesystem::AssetID assetID;

            void SetAssetID(Filesystem::AssetID assetID) {
                this->assetID = assetID;
            }

        private:
            void Init(std::shared_ptr<Shader> shader, bool recievesShadows, RenderMode mode);
            std::map<std::string, UniformValue> parameters;

    };

}