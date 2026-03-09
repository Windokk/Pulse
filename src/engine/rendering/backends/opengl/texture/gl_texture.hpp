#pragma once

#include "engine/rendering/texture/texture.hpp"

namespace Pulse::Engine::Rendering{

    class GLTexture : public Texture{

        public:

            GLTexture(TextureSpecifications& spec, const void* data);

            void Bind(uint32_t slot = 0) const override;

            uint32_t GetHandle() const override { return ID; }

            ~GLTexture();

        private:

            uint32_t ID;

    };

}