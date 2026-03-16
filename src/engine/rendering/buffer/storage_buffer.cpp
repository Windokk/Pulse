#include "storage_buffer.hpp"

#include "engine/core/engine.hpp"
#include "engine/rendering/renderer/renderer.hpp"

#include "engine/rendering/backends/opengl/buffer/gl_storage_buffer.hpp"

namespace Pulse::Engine::Rendering{

    std::shared_ptr<StorageBuffer> StorageBuffer::Create(uint32_t size)
    {
        switch(Core::GetEngine().GetRenderer()->GetRendererAPI()->GetAPI())
        {
            case RendererAPI::API::OpenGL:
                return std::make_shared<GLStorageBuffer>(size);
        }

        return nullptr;
    }
}