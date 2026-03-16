#pragma once

#include "engine/rendering/texture/texture.hpp"

namespace Pulse::Engine::Rendering{

    struct GLTextureSpec{
        
        unsigned int wrapModeS;
        unsigned int wrapModeT;
        unsigned int wrapModeR;

        unsigned int minFilter;
        unsigned int magFilter;

        unsigned int internalFormat;
        unsigned int format;
        
        unsigned int compareFunc;

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