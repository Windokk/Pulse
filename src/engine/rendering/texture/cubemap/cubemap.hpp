#pragma once

#include "engine/rendering/texture/texture.hpp"

namespace Pulse::Engine::Rendering {
    class Cubemap
    {
        public:

            virtual ~Cubemap() = default;

            virtual void Bind(uint32_t slot = 0) const = 0;

            virtual uint32_t GetWidth() const = 0;
            virtual uint32_t GetHeight() const = 0;

            virtual const TextureSpecification& GetSpecification() const = 0;

            static std::shared_ptr<Cubemap> Create(
                const TextureSpecification& spec,
                const std::array<void*,6>& faces
            );
    };

    struct EnvironmentMapInfos{
        int width, height;       
        int nrChannels;      
        std::shared_ptr<Filesystem::Path> filepath;
    };

    class EnvironmentMap
    {
        public:

            EnvironmentMap(const Filesystem::Path& filepath);

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

        private:

            void CreateFromHDR();
            void CreateFromFolder();

            void GenerateIBLMaps();

            Filesystem::AssetID assetID;

            EnvironmentMapInfos infos;

            std::shared_ptr<Cubemap> cubemap;
            std::shared_ptr<Cubemap> irradianceMap;
            std::shared_ptr<Cubemap> prefilterMap;
            std::shared_ptr<Texture> brdfLUT;
    };
}