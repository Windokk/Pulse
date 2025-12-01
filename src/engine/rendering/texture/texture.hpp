#pragma once

#include "engine/ecs/components/core/transform.hpp"
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
        std::shared_ptr<Filesystem::Path> filepath;
    };

    class Texture{
        public:
            Texture(Filesystem::Path filepath);
            void Init(Filesystem::Path filepath);
            void Bind(int unit);
            void UnBind(int unit);
            void Cleanup();
            unsigned int GetID() { return ID; }
            TextureInfos* GetInfos() { return &infos; }

            void SetAssetID(Filesystem::AssetID assetID) {
                this->assetID = assetID;
            }

            Filesystem::AssetID GetAssetID() {
                return this->assetID;
            }

        private:
            Filesystem::AssetID assetID;

            unsigned int ID;
            TextureInfos infos;
            unsigned int VAO, VBO, EBO;
            unsigned int indices[6] = { 0, 1, 2, 2, 3, 0 };
    };
}