#include "debugger.hpp"

#include <iomanip>
#include <ctime>

#include "engine/core/engine.hpp"

namespace Epoch::Engine::Debugging{

    void Debugger::EnableTimestamp()
    {
        useTimestamp = true;
    }

    void Debugger::EnableFileLogging(const std::string &filepath)
    {
        logFile.open(filepath, std::ios::out | std::ios::app);
    }

    void Debugger::SetMinimumLevel(Level level) {
        currentMinLevel = level;
    }

    void Debugger::DebugLog(std::string msg, const char *file, int line)
    {
        Epoch::Engine::Core::GetEngine().GetDebugger()->Log(Epoch::Engine::Debugging::Level::Log, msg, file, line);
    }

    void Debugger::DebugInfo(std::string msg, const char *file, int line)
    {
        Epoch::Engine::Core::GetEngine().GetDebugger()->Log(Epoch::Engine::Debugging::Level::Info, msg, file, line);
    }

    void Debugger::DebugWarning(std::string msg, const char *file, int line)
    {
        Epoch::Engine::Core::GetEngine().GetDebugger()->Log(Epoch::Engine::Debugging::Level::Warning, msg, file, line);
    }

    void Debugger::DebugError(std::string msg, const char *file, int line)
    {
        Epoch::Engine::Core::GetEngine().GetDebugger()->Log(Epoch::Engine::Debugging::Level::Error, msg, file, line);
    }

    void Debugger::DebugFatal(std::string msg, const char *file, int line)
    {
        Epoch::Engine::Core::GetEngine().GetDebugger()->Log(Epoch::Engine::Debugging::Level::Fatal, msg, file, line);
    }

    std::string Debugger::LevelToString(Level level) {
        switch (level) {
            case Level::Log:    return "LOG";
            case Level::Info:   return "INFO";
            case Level::Warning:return "WARNING";
            case Level::Error:  return "ERROR";
            case Level::Fatal:  return "FATAL";
        }
        return "UNKNOWN";
    }

    std::string Debugger::GetTimestamp() {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        localtime_s(&tm, &now_c);

        std::ostringstream oss;
        oss << std::put_time(&tm, "[%Y-%m-%d %H:%M:%S]");
        return oss.str();
    }

    void Debugger::Log(Level level, const std::string& message, const char* file, int line) {
        
        if (level < currentMinLevel)
            return;

        std::string output = (useTimestamp ? GetTimestamp()+" " : "") + "[" + LevelToString(level) + "] (" + file + ":" + std::to_string(line) + ") " + message;

        /*switch(level){
            case Level::Log:
                system("Color 01");
                break;
            case Level::Info:
                system("Color 07");
                break;
            case Level::Warning:
                system("Color 06");
                break;
            case Level::Error:
            case Level::Fatal:
                system("Color 04");
                break;
        }*/

        std::cout << output << std::endl;

        if (logFile.is_open())
            logFile << output << std::endl;

        if (level == Level::Fatal){
            std::cout << "Epoch Engine has crashed. Press Enter to exit..." << std::endl;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cin.get();
            std::terminate(); // crash
        }
            
    }
}