#include "gl_storage_buffer.hpp"

#include "engine/rendering/backends/opengl/gl_utils.hpp"

namespace Pulse::Engine::Rendering{

    GLStorageBuffer::GLStorageBuffer(uint32_t size)
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
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_Buffer);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, size, data);
    }
    
    void GLStorageBuffer::Bind(uint32_t binding)
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, m_Buffer);
    }

}