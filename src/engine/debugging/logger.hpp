#pragma once

#include <string>
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>

namespace Pulse::Engine::Debugging{
    
    enum class Level {
        Log = 0,
        Info = 1,
        Warning = 2,
        Error = 3,
        Fatal = 4
    };

    class Logger {
    public:

        template<typename... Args>
        void Log(Level level, const char* file, int line, Args&&... args) {
            if (static_cast<int>(level) < static_cast<int>(currentMinLevel))
                return;

            std::ostringstream ss;
            ss << "[" << LevelToString(level) << "] ";
            if (useTimestamp)
                ss << GetTimestamp() << " ";
            ss << "(" << file << ":" << line << ") ";
            (ss << ... << args);

            std::string output = ss.str();
            std::cout << output << std::endl;
            if (logFile.is_open())
                logFile << output << std::endl;

            if (level == Level::Fatal) {
                std::cout << "Fatal error! Press Enter to exit..." << std::endl;
                std::cin.get();
                std::terminate();
            }
        }

        void EnableTimestamp();
        void EnableFileLogging(const std::string& filepath);
        void SetMinimumLevel(Level level);

    private:
        std::string LevelToString(Level level);
        std::string GetTimestamp();
        Level currentMinLevel;
        std::ofstream logFile;

        bool useTimestamp;
    };

    // ------------------------ DLL-shared function pointer ------------------------
    using LoggerFuncType = void(*)(int level, const char* file, int line, const char* msg);

    extern LoggerFuncType gLoggerFunc;

    // ------------------------ Variadic template forwarding ------------------------
    template<typename... Args>
    inline void Log(Level level, const char* file, int line, Args&&... args) {
        if (gLoggerFunc) {
            std::ostringstream ss;
            (ss << ... << args);
            gLoggerFunc(static_cast<int>(level), file, line, ss.str().c_str());
        } else {
            // fallback if logger not initialized
            std::ostringstream ss;
            (ss << ... << args);
            std::cerr << "[UNINIT LOGGER] " << ss.str() << std::endl;
        }
    }

}

#define DEBUG_LOG(...)       Pulse::Engine::Debugging::Log(Pulse::Engine::Debugging::Level::Log, __FILE_NAME__, __LINE__, __VA_ARGS__)
#define DEBUG_INFO(...)      Pulse::Engine::Debugging::Log(Pulse::Engine::Debugging::Level::Info, __FILE_NAME__, __LINE__, __VA_ARGS__)
#define DEBUG_WARNING(...)   Pulse::Engine::Debugging::Log(Pulse::Engine::Debugging::Level::Warning, __FILE_NAME__, __LINE__, __VA_ARGS__)
#define DEBUG_ERROR(...)     Pulse::Engine::Debugging::Log(Pulse::Engine::Debugging::Level::Error, __FILE_NAME__, __LINE__, __VA_ARGS__)
#define DEBUG_FATAL(...)     Pulse::Engine::Debugging::Log(Pulse::Engine::Debugging::Level::Fatal, __FILE_NAME__, __LINE__, __VA_ARGS__)