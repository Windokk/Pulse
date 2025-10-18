#pragma once

#include <chrono>
#include <mutex>
#include <thread>

namespace Epoch::Engine::Time{

    struct TimeStamp{
        int milliseconds;
        int seconds;
        int minutes;
        int hours;
        int days;
        
        void Normalize() {
            if (milliseconds >= 1000) {
                seconds += milliseconds / 1000;
                milliseconds %= 1000;
            }

            if (seconds >= 60) {
                minutes += seconds / 60;
                seconds %= 60;
            }

            if (minutes >= 60) {
                hours += minutes / 60;
                minutes %= 60;
            }

            if (hours >= 24) {
                days += hours / 24;
                hours %= 24;
            }
        }
    
        std::string AsString() const {
            std::ostringstream oss;
            oss << days << "d "
                << std::setw(2) << std::setfill('0') << hours << "h:"
                << std::setw(2) << std::setfill('0') << minutes << "m:"
                << std::setw(2) << std::setfill('0') << seconds << "s:"
                << std::setw(3) << std::setfill('0') << milliseconds << "ms";
            return oss.str();
        }
    };

    class TimeManager {
        public:

            void Init(float fixedStep = 1.0f / 60.0f, float maxAccumulated = 4.0f / 60.0f){
                fixedDeltaTime = fixedStep;
                deltaTime = 0.0f;
                lastTime = std::chrono::high_resolution_clock::now();
                appStartTime = lastTime;
                accumulator = 0;
                maxAccumulatedTime = maxAccumulated;
            }

            void Tick() {
                frameStartTime = std::chrono::high_resolution_clock::now();
                auto delta = std::chrono::duration<float>(frameStartTime - lastTime);
                deltaTime = delta.count();
                lastTime = frameStartTime;
            }

            float GetFixedDeltaTime() { return fixedDeltaTime; }
            float GetDeltaTime() { return deltaTime; }

            void SetTimeSpeed(float newSpeed = 1.0f) { timeSpeed = newSpeed; }
            float GetTimeSpeed() { return timeSpeed; }

            TimeStamp CurrentGlobalTime() {
                using namespace std::chrono;

                auto now = system_clock::now();
                auto now_time_t = system_clock::to_time_t(now);
                auto now_ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

                std::tm local_tm;
        #ifdef _WIN32
                localtime_s(&local_tm, &now_time_t);
        #else
                localtime_r(&now_time_t, &local_tm);
        #endif

                TimeStamp ts;
                ts.days = local_tm.tm_yday;
                ts.hours = local_tm.tm_hour;
                ts.minutes = local_tm.tm_min;
                ts.seconds = local_tm.tm_sec;
                ts.milliseconds = static_cast<int>(now_ms.count());

                return ts;
            }

            TimeStamp CurrentAppTime() {
                using namespace std::chrono;

                auto now = high_resolution_clock::now();
                auto elapsed = duration_cast<milliseconds>(now - appStartTime);

                int64_t totalMilliseconds = elapsed.count();

                TimeStamp ts;
                ts.days         = static_cast<int>(totalMilliseconds / (24LL * 60 * 60 * 1000));
                totalMilliseconds %= (24LL * 60 * 60 * 1000);

                ts.hours        = static_cast<int>(totalMilliseconds / (60LL * 60 * 1000));
                totalMilliseconds %= (60LL * 60 * 1000);

                ts.minutes      = static_cast<int>(totalMilliseconds / (60LL * 1000));
                totalMilliseconds %= (60LL * 1000);

                ts.seconds      = static_cast<int>(totalMilliseconds / 1000);
                ts.milliseconds = static_cast<int>(totalMilliseconds % 1000);

                return ts;
            }
            
        private:

            float fixedDeltaTime;
            float deltaTime;

            float accumulator;
            float maxAccumulatedTime;

            std::chrono::high_resolution_clock::time_point lastTime;
            std::chrono::high_resolution_clock::time_point frameStartTime;

            std::chrono::high_resolution_clock::time_point appStartTime;

            float timeSpeed = 1.0f;
    };

            
#if defined(BUILD_EDITOR)

    // Used by the Editor Module DLL
    inline TimeManager* gSharedTimeManagerPtr = nullptr;

    inline void SetTimeManager(TimeManager* ptr) {
        gSharedTimeManagerPtr = ptr;
    }

    inline TimeManager& GetTimeManager() {
        if (!gSharedTimeManagerPtr)
            exit(2);
        return *gSharedTimeManagerPtr;
    }

#endif
}