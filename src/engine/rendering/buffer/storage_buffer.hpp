#pragma once

#include <memory>
#include <cstdint>

namespace Pulse::Engine::Rendering{

    class StorageBuffer
    {
        public:
            virtual ~StorageBuffer() = default;

            virtual void SetData(const void* data, uint32_t size) = 0;
            virtual void Bind(uint32_t binding) = 0;

            static std::shared_ptr<StorageBuffer> Create(uint32_t size);
    };

}