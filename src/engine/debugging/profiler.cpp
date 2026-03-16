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

#include "engine/debugging/logger.hpp"
#include "engine/rendering/renderer/renderer.hpp"
#include "engine/rendering/lighting/light_manager.hpp"
#include "engine/audio/audio_manager.hpp"
#include "engine/levels/level_manager.hpp"
#include "engine/time/time_manager.hpp"
#include "engine/rendering/mesh/mesh.hpp"
#include "engine/core/platform/iplatform.hpp"

namespace Pulse::Engine::Debugging{
   
    Profiler::Profiler(){

        #ifdef _WIN32
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
        #endif
    }

    float Profiler::GetGPUMem()
    {
        #ifdef __WIN32__
            // Windows (PDH)
            pdhStatus = PdhGetFormattedCounterValue(hCounter, PDH_FMT_DOUBLE, NULL, fmtValue);
            if (pdhStatus != ERROR_SUCCESS || fmtValue->CStatus != ERROR_SUCCESS)
                DEBUG_ERROR("PdhGetFormattedCounterValue failed.");

            return static_cast<float>(fmtValue->doubleValue);

        #elif defined(__unix__)

            // detect GPU vendor via sysfs
            std::string vendorPath = "/sys/class/drm/card0/device/vendor";
            std::ifstream vendorFile(vendorPath);

            if (vendorFile.good()) {
                std::string vendorHex;
                vendorFile >> vendorHex;

                // 0x1002 = AMD
                // 0x10de = NVIDIA
                // 0x8086 = Intel
                int vendor = std::stoi(vendorHex, nullptr, 16);

                // AMD path
                if (vendor == 0x1002) {
                    long used = 0;

                    std::ifstream file("/sys/class/drm/card0/device/mem_info_vram_used");
                    if (file.good())
                        file >> used;

                    return used / 1024.0f / 1024.0f; // bytes → MB
                }

                // NVIDIA path (NVML)
                if (vendor == 0x10de) {
        #ifdef USE_NVML
                    nvmlMemory_t mem;
                    if (nvmlDeviceGetMemoryInfo(nvmlDevice, &mem) == NVML_SUCCESS)
                        return mem.used / 1024.0f / 1024.0f;
        #endif
                    return 0.0f; // NVML not enabled
                }

                // Intel (no unified per-process VRAM usage)
                if (vendor == 0x8086) {
                    return 0.0f; // unsupported
                }
            }

            return 0.0f; // fallback

        #endif
    }

    MinimalStatistics Profiler::GetStats()
    {
        MinimalStatistics ret{};

        ret.frameTimeMs = Core::GetEngine().GetTimeManager()->GetDeltaTime() * 1000;
        ret.fps = 1000 / ret.frameTimeMs;

        ret.actors = Core::GetEngine().GetLevelManager()->GetLevelAt(0)->transforms.size();
        ret.lights = Core::GetEngine().GetRenderer()->GetLightManager()->GetLightsCount();

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
        #ifdef __WIN32__
            if (hQuery)
                PdhCloseQuery (hQuery);
        #endif
    }
}