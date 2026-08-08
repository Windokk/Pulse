#pragma once

#ifdef __WIN32__

    struct _PDH_FMT_COUNTERVALUE;
    using PDH_FMT_COUNTERVALUE = _PDH_FMT_COUNTERVALUE;

#endif

#include <cstdint>
#include <string>
#include <vector>

namespace Pulse::Engine::Debugging{
    
    struct MinimalStatistics {
        //Audio
        int sounds = 0;

        /// Rendering
        float frameTimeMs = 0;
        float fps = 0;

        ////// Draw list
        int cmds = 0;
        int primitives = 0;
        int vertices = 0;

        ////// Memory usage
        float gpuMemoryMB = 0;

        //Level
        int actors = 0;
        int lights = 0;
    };

    struct SystemCapabilities
    {
        // Feature Support
        bool supportHardwareRayTracing;
        bool supportMeshShaders;
        bool supportTaskShaders;
        bool supportBindlessTextures;
        bool supportVariableRateShading;
        bool supportSamplerAnisotropy;
        bool supportConservativeRasterization;
        bool supportMultiDrawIndirect;
        bool supportIndirectFirstInstance;
        bool supportDrawIndirectCount;
        bool supportPipelineStatisticsQuery;
        bool supportTimestampQuery;
        bool supportOcclusionQuery;
        bool supportTextureCompressionBC;
        bool supportTextureCompressionASTC;
        bool supportTextureCompressionETC2;
        bool supportCubeMapArrays;
        bool supportArrayTextures;
        bool support3DTextures;
        bool supportDepthClamp;
        bool supportDepthBiasClamp;
        bool supportWideLines;
        bool supportLargePoints;
        bool supportGeometryShaders;
        bool supportTessellationShaders;
        bool supportComputeShaders;

        // Texture Limits
        uint32_t maxTexture1DSize;
        uint32_t maxTexture2DSize;
        uint32_t maxTexture3DSize;
        uint32_t maxTextureCubeSize;
        uint32_t maxTextureArrayLayers;
        uint32_t maxTextures;
        uint32_t maxSamplers;
        uint32_t maxAnisotropy;

        // Buffer Limits
        uint32_t maxUniformBufferSize;
        uint32_t maxStorageBufferSize;
        uint32_t maxPushConstants;
        uint32_t maxVertexBufferBindings;
        uint32_t maxVertexAttributes;
        uint32_t maxVertexBufferStride;

        // Shader Limits
        uint32_t maxComputeSharedMemorySize;
        uint32_t maxComputeWorkGroupInvocations;
        uint32_t maxComputeWorkGroupSizeX;
        uint32_t maxComputeWorkGroupSizeY;
        uint32_t maxComputeWorkGroupSizeZ;

        // Rendering Limits
        uint32_t maxColorAttachments;
        uint32_t maxRenderTargets;
        uint32_t maxFramebufferWidth;
        uint32_t maxFramebufferHeight;
        uint32_t maxFramebufferLayers;
        uint32_t maxViewports;
        uint32_t maxScissors;

        // Draw / Scene Limits
        uint32_t maxDrawCallsPerFrame;
        uint32_t maxInstancesPerDraw;
        uint32_t maxVerticesPerMesh;
        uint32_t maxIndicesPerMesh;
        uint32_t maxMeshes;
        uint32_t maxMaterials;
        uint32_t maxLights;

        // Ray Tracing Limits
        uint32_t maxRayRecursionDepth;
        uint32_t maxAccelerationStructures;
        uint32_t maxShaderGroups;

        // Memory / GPU Limits
        uint64_t maxGpuMemory;
        uint64_t maxBufferAllocationSize;
        uint64_t maxTextureAllocationSize;
    };

    class Profiler {
        public:
            Profiler();
            float GetGPUMem();
            MinimalStatistics GetStats();
            void Shutdown();
        private:

        #ifdef __WIN32__
            void* hQuery = nullptr;
            long pdhStatus;
            std::string gpuCounterPathPattern;
            std::vector<void*> gpuCounters;
            bool gpuCountersBound = false;
            PDH_FMT_COUNTERVALUE* fmtValue;
        #endif
    };
}