#pragma once

#include "engine/rendering/texture/texture.hpp"

namespace Pulse::Engine::Rendering {
    class Cubemap
    {
        public:

            virtual ~Cubemap() = default;

            virtual void Bind(uint32_t slot = 0) const = 0;

            uint32_t GetWidth() const { return m_Specifications.width; }
            uint32_t GetHeight() const { return m_Specifications.height; }

            virtual uint32_t GetHandle() const = 0;
            
            virtual const TextureSpecifications& GetSpecification() const = 0;

            static std::shared_ptr<Cubemap> Create(
                const TextureSpecifications& spec,
                const std::array<void*,6>& faces
            );
        protected:
            TextureSpecifications m_Specifications;
    };

    struct EnvironmentMapInfos{
        int width, height;       
        int nrChannels;      
        std::shared_ptr<Filesystem::Path> filepath;
    };

    class EnvironmentMap
    {
        public:

            static std::shared_ptr<Cubemap> Create(
                const TextureSpecifications& spec,
                const std::vector<Filesystem::Path> imageFiles
            );

            static std::shared_ptr<Cubemap> Create(
                const TextureSpecifications& spec,
                const Filesystem::Path hdrFile
            );

            const EnvironmentMapInfos& GetInfos() const { return infos; }

            std::shared_ptr<Cubemap> GetCubemap() const { return cubemap; }
            std::shared_ptr<Cubemap> GetIrradiance() const { return irradianceMap; }
            std::shared_ptr<Cubemap> GetPrefilter() const { return prefilterMap; }
            std::shared_ptr<Texture> GetBRDFLUT() const { return brdfLUT; }

            void SetAssetID(Filesystem::AssetID assetID)
            {
                this->assetID = assetID;
            }

            Filesystem::AssetID GetAssetID() const
            {
                return assetID;
            }

        protected:

            virtual void CreateFromHDR();
            virtual void CreateFromFiles();

            virtual void GenerateIBLMaps();

            Filesystem::AssetID assetID;

            EnvironmentMapInfos infos;

            std::shared_ptr<Cubemap> cubemap;
            std::shared_ptr<Cubemap> irradianceMap;
            std::shared_ptr<Cubemap> prefilterMap;
            std::shared_ptr<Texture> brdfLUT;
    };
}