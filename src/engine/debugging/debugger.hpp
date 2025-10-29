#pragma once

#include <string>
#include <chrono>
#include <iostream>
#include <fstream>

namespace Pulse::Engine::Debugging{
    enum class Level {
        Log = 0,
        Info = 1,
        Warning = 2,
        Error = 3,
        Fatal = 4
    };

    class Debugger {
    public:

        void Log(Level level, const std::string& message, const char* file, int line);

        void EnableTimestamp();
        void EnableFileLogging(const std::string& filepath);
        void SetMinimumLevel(Level level);

        static void DebugLog(std::string msg, const char *file, int line);
        static void DebugInfo(std::string msg, const char *file, int line);
        static void DebugWarning(std::string msg, const char *file, int line);
        static void DebugError(std::string msg, const char *file, int line);
        static void DebugFatal(std::string msg, const char *file, int line);

    private:
        std::string LevelToString(Level level);
        std::string GetTimestamp();
        Level currentMinLevel;
        std::ofstream logFile;

        bool useTimestamp;
    };

}

#define DEBUG_LOG(msg)       Pulse::Engine::Debugging::Debugger::DebugLog(msg, __FILE_NAME__, __LINE__);
#define DEBUG_INFO(msg)      Pulse::Engine::Debugging::Debugger::DebugInfo(msg, __FILE_NAME__, __LINE__);
#define DEBUG_WARNING(msg)   Pulse::Engine::Debugging::Debugger::DebugWarning(msg, __FILE_NAME__, __LINE__);
#define DEBUG_ERROR(msg)     Pulse::Engine::Debugging::Debugger::DebugError(msg, __FILE_NAME__, __LINE__);
#define DEBUG_FATAL(msg)     Pulse::Engine::Debugging::Debugger::DebugFatal(msg, __FILE_NAME__, __LINE__);