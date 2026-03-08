#pragma once

#include "engine/ecs/components/misc/transform.hpp"
#include "engine/filesystem/assetID.hpp"

#include <stb/stb_image.h>
#include <stb/stb_image_resize2.h>
#include <string>
#include <memory>

namespace Pulse::Engine::Filesystem{

    class Path;
}

namespace Pulse::Engine::Rendering {

    struct DrawCommand;

    struct TextureInfos{
        int width, height;           
        int nrChannels;
    };

    class Texture{
        public:
            Texture(int width, int height, int nrChannels, GLenum filterMode[2], GLenum wrapMode[2], GLenum internalFormat, GLenum format, unsigned char *data, bool generateMipmap);
            void Bind(int unit);
            void UnBind(int unit);
            void Cleanup();
            unsigned int GetID() { return ID; }
            TextureInfos* GetInfos() { return &infos; }


        private:

            unsigned int ID;
            TextureInfos infos;
    };

    class Image{
        public:
            Image(Filesystem::Path filepath);
        
            std::shared_ptr<Texture> texture;

            GLenum wrapModes[2] = {GL_REPEAT, GL_REPEAT};
            GLenum filterModes[2] = {GL_LINEAR, GL_LINEAR};

            void SetAssetID(Filesystem::AssetID assetID) {
                this->assetID = assetID;
            }

            Filesystem::AssetID GetAssetID() {
                return this->assetID;
            }
        private:
            Filesystem::AssetID assetID;
            
    };
}