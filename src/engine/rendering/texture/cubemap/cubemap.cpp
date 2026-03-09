#include "cubemap.hpp"

#include "engine/core/engine.hpp"

#include "engine/rendering/backends/opengl/texture/cubemap/gl_cubemap.hpp"

#include <stb/stb_image.h>

namespace Pulse::Engine::Rendering{
    std::shared_ptr<Cubemap> Cubemap::Create(const TextureSpecifications& specs, std::array<unsigned char*, 6> faces)
    {
        switch(Core::GetEngine().GetRenderer()->GetRendererAPI())
        {
            case RendererAPI::API::OpenGL:
                return std::make_shared<GLCubemap>(specs, faces);

            /*case API::Vulkan:
                return std::make_shared<VKCubemap>(faces);

            case API::DX11:
                return std::make_shared<DX11Cubemap>(faces);

            case API::DX12:
                return std::make_shared<DX12Cubemap>(faces);*/

            default:
                return nullptr;
        }
    }

    std::shared_ptr<Cubemap> Cubemap::Create(TextureSpecifications& specs, const std::vector<Filesystem::Path> imageFiles)
    {
        if (imageFiles.size() != 6)
        return nullptr;

        std::array<unsigned char*, 6> faces;

        int width, height, channels;

        for (size_t i = 0; i < 6; i++)
        {
            Filesystem::Path filePath = imageFiles[i];

            if(!filePath.Exists() || filePath.IsDirectory()){
                DEBUG_ERROR("Attempted to create a cubemap with a missing file : ",filePath.full);
                return nullptr;
            }

            std::string file = filePath.ReadFile();
            
            faces[i] = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(file.data()),
                                            static_cast<int>(file.size()),
                                            &width, &height, &channels, 0);

            if (!faces[i])
            {
                for (size_t j = 0; j < i; j++)
                    stbi_image_free(faces[j]);

                return nullptr;
            }
        }

        auto cubemap = Create(specs, faces);

        //TODO : Set specifications from data
        // specs.width = 
        // specs.height = 
        // specs.format = 

        for (int i = 0; i < 6; i++)
            stbi_image_free(faces[i]);

        return cubemap;
    }
}