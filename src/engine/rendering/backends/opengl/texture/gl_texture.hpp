#pragma once

#include "engine/rendering/texture/texture.hpp"

namespace Pulse::Engine::Rendering{

    struct GLTextureSpec{
        
        uint32_t wrapModeS;
        uint32_t wrapModeT;
        uint32_t wrapModeR;

        uint32_t minFilter;
        uint32_t magFilter;

        uint32_t internalFormat;
        uint32_t format;
        
        uint32_t compareFunc;
        
        uint32_t type;

        static GLTextureSpec FromTextureSpecifications(const TextureSpecifications& spec);
    };

    class GLTexture2D : public Texture2D{

        public:

            GLTexture2D(TextureSpecifications& specs, const void* data);

            void Bind(uint32_t slot = 0) const override;

            bool IsValid() const override;

            uint32_t GetHandle() const override { return ID; }

            ~GLTexture2D();

        private:

            uint32_t ID;

    };

}