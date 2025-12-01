#include "logger.hpp"

#include <iomanip>
#include <ctime>

#include "engine/core/engine.hpp"

namespace Pulse::Engine::Debugging{

    LoggerFuncType gLoggerFunc = nullptr;

    void Logger::EnableTimestamp()
    {
        useTimestamp = true;
    }

    void Logger::EnableFileLogging(const std::string &filepath)
    {
        logFile.open(filepath, std::ios::out | std::ios::app);
    }

    void Logger::SetMinimumLevel(Level level) {
        currentMinLevel = level;
    }

    std::string Logger::LevelToString(Level level) {
        switch (level) {
            case Level::Log:    return "LOG";
            case Level::Info:   return "INFO";
            case Level::Warning:return "WARNING";
            case Level::Error:  return "ERROR";
            case Level::Fatal:  return "FATAL";
        }
        return "UNKNOWN";
    }

    std::string Logger::GetTimestamp()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        localtime_s(&tm, &now_c);

        std::ostringstream oss;
        oss << std::put_time(&tm, "[%Y-%m-%d %H:%M:%S]");
        return oss.str();
    }
}