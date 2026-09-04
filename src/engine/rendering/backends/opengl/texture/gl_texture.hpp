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

            void BindImage(uint32_t unit, TextureAccess access, uint32_t level = 0) const override;

            void ReadPixels(void* outData, size_t bufferSize, uint32_t level = 0) const override;

            bool IsValid() const override;

            uint32_t GetHandle() const override { return ID; }

            // Returns a resident ARB_bindless_texture handle for the given raw GL texture ID, creating
            // and residency-registering it on first use (cached thereafter). Returns 0 if glTextureID is
            // 0 or GL_ARB_bindless_texture isn't supported by the driver - callers should treat 0 as
            // "no texture available" and fall back to flat scalar values instead.
            static uint64_t GetBindlessHandle(uint32_t glTextureID);

            ~GLTexture2D();

        private:

            uint32_t ID;

            uint32_t m_GLInternalFormat = 0;

    };

}