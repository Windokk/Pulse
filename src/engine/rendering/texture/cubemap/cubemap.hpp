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

            const TextureSpecifications& GetSpecification() const { return m_Specifications; }

            virtual uint32_t GetHandle() const = 0;

            static std::shared_ptr<Cubemap> Create(const TextureSpecifications& specs, std::array<unsigned char*, 6>);

            static std::shared_ptr<Cubemap> Create(TextureSpecifications& specs, const std::vector<Filesystem::Path> imageFiles);

        protected:
            TextureSpecifications m_Specifications;
    };
}