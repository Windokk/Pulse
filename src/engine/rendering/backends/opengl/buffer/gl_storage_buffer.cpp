#include "gl_storage_buffer.hpp"

#include "engine/rendering/backends/opengl/gl_utils.hpp"

namespace Pulse::Engine::Rendering{

    GLStorageBuffer::GLStorageBuffer(uint32_t size) : m_Size(size)
    {
        glGenBuffers(1, &m_Buffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_Buffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
    }

    GLStorageBuffer::~GLStorageBuffer()
    {
        glDeleteBuffers(1, &m_Buffer);
    }

    void GLStorageBuffer::SetData(const void *data, uint32_t size)
    {
        m_Size = size;
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_Buffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, size, data);
    }

    void GLStorageBuffer::GetData(void* outData, uint32_t size, uint32_t offset) const
    {
        if ((uint64_t)offset + (uint64_t)size > (uint64_t)m_Size)
        {
            DEBUG_ERROR("GetData range [" + std::to_string(offset) + ", " + std::to_string(offset + size) + ") exceeds storage buffer size (" + std::to_string(m_Size) + ") - buffer was not read back.");
            return;
        }

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_Buffer);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, outData);
    }

    void GLStorageBuffer::Bind(uint32_t binding)
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, m_Buffer);
    }

}