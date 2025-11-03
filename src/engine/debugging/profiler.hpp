#pragma once

#ifdef __WIN32__

    struct _PDH_FMT_COUNTERVALUE;
    using PDH_FMT_COUNTERVALUE = _PDH_FMT_COUNTERVALUE;

#endif

namespace Pulse::Engine::Debugging{
    
    struct MinimalStatistics {
        //Audio
        int sounds = 0;

        //Rendering
        float frameTimeMs = 0;
        float fps = 0;
        int drawCalls = 0;
        int triangles = 0;
        int vertices = 0;
        float gpuMemoryMB = 0;

        //Level
        int actors = 0;
        int lights = 0;
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
            void* hCounter;
            PDH_FMT_COUNTERVALUE* fmtValue;
        #endif
    };
}