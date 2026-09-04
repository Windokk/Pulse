#include "gl_compute_pipeline.hpp"

#include "engine/rendering/backends/opengl/gl_utils.hpp"

#include "engine/rendering/backends/opengl/shader/gl_compute_shader.hpp"

namespace Pulse::Engine::Rendering{
    
    GLComputePipeline::GLComputePipeline(const ComputePipelineSpecifications& specs)
    {
        m_Specifications = specs;
    }

    void GLComputePipeline::Bind()
    {
        std::shared_ptr<GLComputeShader> glShader = std::static_pointer_cast<GLComputeShader>(m_Specifications.shader);
        glShader->Bind();
    }
}