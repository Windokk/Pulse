#pragma once

#include "engine/rendering/utils.hpp"

#include "engine/filesystem/filesystem.hpp"

namespace Pulse::Engine::Rendering {
    
    enum class TextureFilter
    {
        Linear,
        Nearest
    };

    enum class TextureWrap
    {
        Repeat,
        Clamp,
        Mirror
    };

    enum class TextureFormat
    {
        R8,
        RG8,
        RGB8,
        RGBA8,
        RGBA16F,
        Depth24Stencil8
    };

    struct TextureSpecification
    {
        uint32_t width = 0;
        uint32_t height = 0;

        TextureFormat format = TextureFormat::RGBA8;

        TextureFilter minFilter = TextureFilter::Linear;
        TextureFilter magFilter = TextureFilter::Linear;

        TextureWrap wrapS = TextureWrap::Repeat;
        TextureWrap wrapT = TextureWrap::Repeat;

        bool generateMips = true;
    };

    class Texture
    {
        public:

            virtual ~Texture() = default;

            virtual void Bind(uint32_t slot = 0) const = 0;

            virtual uint32_t GetWidth() const = 0;
            virtual uint32_t GetHeight() const = 0;

            virtual const TextureSpecification& GetSpecification() const = 0;

            static std::shared_ptr<Texture> Create(
                const TextureSpecification& spec,
                const void* data
            );

            static std::shared_ptr<Texture> Create(
                TextureSpecification& spec,
                Filesystem::Path& filepath
            );

            void SetAssetID(Filesystem::AssetID assetID) {
                this->assetID = assetID;
            }

            Filesystem::AssetID GetAssetID() {
                return this->assetID;
            }
        private:
            Filesystem::AssetID assetID;
    };

}