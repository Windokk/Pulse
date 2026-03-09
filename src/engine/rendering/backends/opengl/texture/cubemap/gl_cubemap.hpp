#pragma once

#include "engine/rendering/texture/cubemap/cubemap.hpp"

namespace Pulse::Engine::Rendering{

    class GLCubemap : public Cubemap{
        public:

            GLCubemap(const TextureSpecifications& spec, const std::array<void*,6>& faces);

            void Bind(uint32_t slot = 0) const override;

            ~GLCubemap() override;

            uint32_t GetHandle() const override { return ID; }

        private:
            uint32_t ID;
    };

    class GLEnvironmentMap : public EnvironmentMap{
        public:
            GLEnvironmentMap(const EnvironmentMapInfos& infos, const std::array<void*,6>& faces);
            GLEnvironmentMap(const EnvironmentMapInfos& infos, const void* hdrData);
        private:

    };

}