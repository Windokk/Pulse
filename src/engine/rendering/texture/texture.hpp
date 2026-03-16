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
        ClampEdge,
        ClampBorder,
        Mirror
    };

    enum class TextureInternalFormat
    {
        RED,
        R16F,
        RG,
        RG16F,
        RGB,
        RGB16F,
        RGB32I,
        RGB32F,
        RGBA8,
        RGBA,
        RGBAF,
        RGBA32I,
        RGBA32F,
        Depth24Stencil8,
        Depth16,
        Depth24,
        Depth32,
        Depth32F
    };

    enum class TextureFormat
    {
        RED,
        RG,
        RGB,
        RGBA,
        Depth
    };

    enum class TextureCompareFunc
    {
        Less,
        LessOrEqual,
        Greater,
        Always,
        Never
    };

    enum class TextureCompareMode
    {
        None,
        CompareRefToTexture
    };

    struct TextureSpecifications
    {
        uint32_t width = 0;
        uint32_t height = 0;

        TextureFormat format = TextureFormat::RGBA;
        TextureInternalFormat internalFormat = TextureInternalFormat::RGBA;

        TextureFilter minFilter = TextureFilter::Linear;
        TextureFilter magFilter = TextureFilter::Linear;

        TextureWrap wrapS = TextureWrap::Repeat;
        TextureWrap wrapT = TextureWrap::Repeat;
        TextureWrap wrapR = TextureWrap::Repeat;

        COL_RGBA borderColor = COL_RGBA(-1.0f);

        TextureCompareFunc compareFunc;
        TextureCompareMode compareMode;

        bool generateMips = true;
    };

    class Texture2D
    {
        public:

            virtual ~Texture2D() = default;

            virtual void Bind(uint32_t slot = 0) const = 0;

            uint32_t GetWidth() const { return m_Specifications.width; }
            uint32_t GetHeight() const { return m_Specifications.height; }

            virtual uint32_t GetHandle() const = 0;

            virtual bool IsValid() const = 0;

            const TextureSpecifications& GetSpecifications() const { return m_Specifications; };

            static std::shared_ptr<Texture2D> Create(
                TextureSpecifications& spec,
                const void* data
            );

            static std::shared_ptr<Texture2D> Create(
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