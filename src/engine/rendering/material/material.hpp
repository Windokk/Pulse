#pragma once

#include <variant>
#include <stdexcept>
#include <memory>

namespace Pulse::Engine::Rendering {

    class Shader;

    class CommandBuffer;

    enum RenderMode{
        OPAQUE,
        TRANSLUCENT,
        MASKED
    };

    using NumericValue = std::variant<bool, float, int, glm::vec2, glm::vec3, glm::vec4, glm::mat4>;

    class Material {
        public:

            virtual ~Material() = default;

            virtual void SetScalarParameter(
                const std::string& name,
                const NumericValue& value) = 0;

            virtual void SetTextureParameter(
                const std::string& name,
                uint64_t texture) = 0;

            virtual std::optional<NumericValue>
            GetScalarParameter(const std::string& name) = 0;

            virtual uint32_t GetTexturesCount() const = 0;

            virtual void Bind(CommandBuffer& cmd) = 0;

            void SetAssetID(Filesystem::AssetID assetID) {
                this->assetID = assetID;
            }

            Filesystem::AssetID GetAssetID() {
                return assetID;
            }

        protected:

            Filesystem::AssetID assetID;

        public:

            std::shared_ptr<Shader> shader;
            bool receivesShadows = false;
            RenderMode renderMode = OPAQUE;
    };

}