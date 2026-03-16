#pragma once

#include "engine/rendering/buffer/storage_buffer.hpp"

namespace Pulse::Engine::Rendering{

    class GLStorageBuffer : public StorageBuffer
    {
        public:
            GLStorageBuffer(uint32_t size);
            
            ~GLStorageBuffer();

            void SetData(const void* data, uint32_t size) override;

            void Bind(uint32_t binding) override;

        private:
            uint32_t m_Buffer;
    };
}