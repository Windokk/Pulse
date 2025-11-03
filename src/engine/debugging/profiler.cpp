#include "profiler.hpp"

#include "engine/core/engine.hpp"

#ifdef __WIN32__
#define byte cs_byte
#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <iostream>
#include <vector>
#include <string>

#ifdef byte
#undef byte
#endif


#elif defined(__unix__)

#endif

namespace Pulse::Engine::Debugging{
   
    Profiler::Profiler(){
        std::string counterMBPath = std::string("\\GPU Process Memory(pid_") +
                                 std::to_string(getpid()) +
                                 "*)\\Dedicated Usage";

        fmtValue = new PDH_FMT_COUNTERVALUE;

        pdhStatus = PdhOpenQuery(NULL, 0, &hQuery);
        if (pdhStatus != ERROR_SUCCESS)
        {
            DEBUG_ERROR(L"PdhOpenQuery failed with 0x%x", pdhStatus);
        }

        // Add counters
        pdhStatus = PdhAddEnglishCounterA(hQuery,
            counterMBPath.c_str(),
            0,
            &hCounter);

        if (pdhStatus != ERROR_SUCCESS)
        {
            DEBUG_ERROR(L"PdhAddCounter failed with 0x%x", pdhStatus);
        }
    }

    float Profiler::GetGPUMem(){
        
        // Get formatted value (as double)
        pdhStatus = PdhGetFormattedCounterValue(hCounter, PDH_FMT_DOUBLE, NULL, fmtValue);
        if (pdhStatus != ERROR_SUCCESS || fmtValue->CStatus != ERROR_SUCCESS)
        {
            DEBUG_ERROR("PdhGetFormattedCounterValue for counter : GPU Mem MB failed : pdh=0x%08x cstatus=0x%08x", pdhStatus, fmtValue->CStatus);
        }

        return static_cast<float>(fmtValue->doubleValue);
    }

    MinimalStatistics Profiler::GetStats()
    {
        MinimalStatistics ret{};

        for(auto& cmd : *Core::GetEngine().GetRenderer()->GetDrawList()){
            ret.drawCalls++;
            ret.triangles += cmd.indexCount / 3;
            ret.vertices += cmd.verticesCount;
        }

        ret.frameTimeMs = Core::GetEngine().GetTimeManager()->GetDeltaTime() * 1000;
        ret.fps = 1000 / ret.frameTimeMs;

        ret.actors = Core::GetEngine().GetLevelManager()->GetLevelAt(0)->transforms.size();
        ret.lights = Core::GetEngine().GetRenderer()->lightMan->GetLightsCount();

        ret.sounds = Core::GetEngine().GetAudioManager()->GetSoundsCount();

        Core::Platform::SystemInfos infos = Core::GetEngine().GetWindow()->GetSystemInfos();

        #ifdef __WIN32__
        {
            // Initial sample.
            pdhStatus = PdhCollectQueryData(hQuery);
            if (pdhStatus != ERROR_SUCCESS)
            {
                DEBUG_ERROR("PdhCollectQueryData (initial) failed with 0x%08x", pdhStatus);
            }

            ret.gpuMemoryMB = GetGPUMem() / (1024 * 1024);
        }

        #elif defined(__unix__)
        {

        }
        #endif

        return ret;
    }

    void Profiler::Shutdown()
    {
        // Close the query object
        if (hQuery)
            PdhCloseQuery (hQuery);
    }
}