#include "texture.hpp"

#include "engine/core/engine.hpp"

#include <stb/stb_image.h>

namespace Pulse::Engine::Rendering{

    std::shared_ptr<Texture> Texture::Create(const TextureSpecification &spec, const void *data)
    {
        switch(Core::GetEngine().GetRenderer()->GetRendererAPI())
        {
            case RendererAPI::API::OpenGL:
                return std::make_shared<GLTexture>(spec, data);

            /*case RendererAPI::API::Vulkan:
                return std::make_shared<VKTexture>(spec, data);

            case RendererAPI::API::DX11:
                return std::make_shared<DX11Texture>(spec, data);

            case RendererAPI::API::DX12:
                return std::make_shared<DX12Texture>(spec, data);*/

            default:
                return nullptr;
        }
    }

    std::shared_ptr<Texture> Texture::Create(TextureSpecification &spec, Filesystem::Path &filepath)
    {
        void* data = nullptr;

        int width, height, nrChannels;
        TextureFormat format;

        if (filepath.Exists()) {
            std::string file = filepath.ReadFile();
            
            stbi_set_flip_vertically_on_load(true);
            data = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(file.data()),
                                            static_cast<int>(file.size()),
                                            &width, &height, &nrChannels, 0);

        } else {
            DEBUG_ERROR("Couldn't find texture : " + filepath.full);
            return;
        }

        if (data) {
            if (nrChannels == 1){
                format = TextureFormat::R8;
            }
            else if (nrChannels == 2){
                format = TextureFormat::RG8;
            }
            else if (nrChannels == 3){
                format = TextureFormat::RGB8;
            }
            else if (nrChannels == 4){
                format = TextureFormat::RGBA16F;
            }
            else{
                format = TextureFormat::RGB8; // Default to RGB
            }

        } else {
            DEBUG_ERROR("Couldn't load texture : " + filepath.full);   
        }

        spec.format = format;
        spec.width = width;
        spec.height = height;

        return Create(spec, data);

        stbi_image_free(data);
    }
}