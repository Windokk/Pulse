#pragma once

#include "engine/rendering/pipeline/pipeline.hpp"

namespace Pulse::Engine::Rendering{
    class GLPipeline : public Pipeline
    {
    public:

        GLPipeline(const PipelineSpecifications& specs);

        void Invalidate() override;

        void Bind() override;
        
    private:

    };
}