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
        RED,
        RG,
        RGB,
        RGBA,
        RGBA,
        Depth24Stencil8
    };

    struct TextureSpecifications
    {
        uint32_t width = 0;
        uint32_t height = 0;

        TextureFormat format = TextureFormat::RGBA;

        TextureFilter minFilter = TextureFilter::Linear;
        TextureFilter magFilter = TextureFilter::Linear;

        TextureWrap wrapS = TextureWrap::Repeat;
        TextureWrap wrapT = TextureWrap::Repeat;
        TextureWrap wrapR = TextureWrap::Repeat;

        bool generateMips = true;
    };

    class Texture
    {
        public:

            virtual ~Texture() = default;

            virtual void Bind(uint32_t slot = 0) const = 0;

            uint32_t GetWidth() const { return m_Specifications.width; }
            uint32_t GetHeight() const { return m_Specifications.height; }

            virtual uint32_t GetHandle() const = 0;

            const TextureSpecifications& GetSpecifications() const { return m_Specifications; };

            static std::shared_ptr<Texture> Create(
                const TextureSpecifications& spec,
                const void* data
            );

            static std::shared_ptr<Texture> Create(
                TextureSpecifications& spec,
                const Filesystem::Path& filepath
            );

            void SetAssetID(Filesystem::AssetID assetID) {
                this->assetID = assetID;
            }

            Filesystem::AssetID GetAssetID() {
                return this->assetID;
            }
        protected:
            Filesystem::AssetID assetID;

            TextureSpecifications m_Specifications;
    };

}