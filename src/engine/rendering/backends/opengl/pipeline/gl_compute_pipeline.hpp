#pragma once

#include "engine/rendering/pipeline/compute_pipeline.hpp"

namespace Shard::Engine::Rendering{
    class GLComputePipeline : public ComputePipeline
    {
    public:

        GLComputePipeline(const ComputePipelineSpecifications& specs);

        void Bind() override;

    private:

    };
}