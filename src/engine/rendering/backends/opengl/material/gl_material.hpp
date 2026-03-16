#pragma once

#include "engine/rendering/material/material.hpp"

#include <unordered_map>


namespace Pulse::Engine::Rendering{

    class GLMaterial : public Material
    {
    public:

        GLMaterial(std::shared_ptr<Shader> shader, std::shared_ptr<Pipeline> pipeline, bool receivesShadows, Opacity opacity);

        virtual ~GLMaterial() = default;

        void SetScalarParameter(
            const std::string& name,
            const NumericValue& value) override;

        void SetTextureParameter(
            const std::string& name,
            uint64_t texture) override;

        std::optional<NumericValue>
        GetScalarParameter(const std::string& name) override;

        uint32_t GetTexturesCount() const override;

        void Bind();

    private:

    };
}