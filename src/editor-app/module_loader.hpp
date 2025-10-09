#pragma once

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #define NOCRYPT
    #define NORPC
    #include <windows.h>
    using ModuleHandle = HMODULE;
#else
    #include <dlfcn.h>
    using ModuleHandle = void*;
#endif

#include "engine/debugging/debugger.hpp"

#include "engine/ecs/components/core/registry/component_registry.hpp"

namespace Epoch::Launcher {

    using namespace Epoch::Engine;
    using namespace Epoch::Engine::Core;

    using CreatePlatformFn = Platform::IPlatform* (*)();
    
    // EDITOR
    using EditorInitFn = void(*)(EngineInstance*, Debugging::Debugger*, Rendering::Renderer*, Resources::ResourcesManager*, Rendering::CameraManager*, Time::TimeManager*, OpenGL*);
    using EditorStartFn = void(*)();
    using EditorTickFn = void(*)();
    using EditorCleanupFn = void (*)();

    //GAME
    using GameInitFn = void(*)(EngineInstance*, Debugging::Debugger*, Debugging::Level, ECS::Components::ComponentRegistry*, OpenGL*);
    using GameRegisterComponentsFn = void(*)();

    class ModuleLoader {
    public:
        static ModuleLoader& GetInstance() {
            static ModuleLoader instance;
            return instance;
        }

        bool LoadModule(const std::string& name, const std::string& path) {
            if (modules.find(name) != modules.end()) return true;

            ModuleHandle handle;

        #if defined(_WIN32)
            handle = LoadLibraryA(path.c_str());
            if (!handle) {
                DEBUG_FATAL("Failed to load module: " + path);
                return false;
            }
        #else
            handle = dlopen(path.c_str(), RTLD_NOW);
            if (!handle) {
                DEBUG_FATAL("Failed to load module: " + std::string(dlerror()));
                return false;
            }
        #endif

            modules[name] = handle;
            DEBUG_LOG("Module loaded: " + path);
            return true;
        }

        template<typename T>
        T GetSymbol(const std::string& moduleName, const std::string& symbolName) {
            if (modules.find(moduleName) == modules.end()) {
                DEBUG_FATAL("Module not loaded: " + moduleName);
                return nullptr;
            }

            ModuleHandle handle = modules[moduleName];

        #if defined(_WIN32)
            auto symbol = reinterpret_cast<T>(GetProcAddress(handle, symbolName.c_str()));
        #else
            auto symbol = reinterpret_cast<T>(dlsym(handle, symbolName.c_str()));
        #endif

            if (!symbol) {
                DEBUG_FATAL("Symbol not found: " + symbolName);
            }

            return symbol;
        }

        void UnloadModule(const std::string& moduleName) {
            if (modules.find(moduleName) == modules.end()) return;

        #if defined(_WIN32)
            FreeLibrary(modules[moduleName]);
        #else
            dlclose(modules[moduleName]);
        #endif

            modules.erase(moduleName);
            DEBUG_LOG("Unloaded module: " + moduleName);
        }

        bool IsModuleLoaded(const std::string& moduleName) {
            return modules.find(moduleName) != modules.end();
        }

    private:
        std::unordered_map<std::string, ModuleHandle> modules;

        ModuleLoader() = default;
        ~ModuleLoader() = default;
        ModuleLoader(const ModuleLoader&) = delete;
        ModuleLoader& operator=(const ModuleLoader&) = delete;
    };
}