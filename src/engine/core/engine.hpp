#pragma once

#include <stdexcept>

#include "engine/filesystem/filesystem.hpp"
#include "engine/levels/level_manager.hpp"
#include "engine/ecs/objects/actors/actor.hpp"
#include "engine/core/platform/iplatform.hpp"
#include "engine/time/time_manager.hpp"

namespace Epoch::Engine::Core {
    
    struct EngineCreationSettings{
        //PLATFORM
        Platform::IPlatform* platform = nullptr;

        //WINDOW
        int windowWidth = 800;
        int windowHeight = 800;
        bool fullscreen = false;
        bool vsync = true;
        int targetFPS = 60;

        //FILESYSTEM
        std::string rootPath = "";

        //PHYSICS
        glm::vec3 gravity = glm::vec3(0, -9.81f, 0);
    };

    class EngineInstance {
        public:
            // Delete copy constructor and assignment operator to enforce singleton
            EngineInstance(const EngineInstance&) = delete;
            EngineInstance& operator=(const EngineInstance&) = delete;

            // Provide access to the singleton instance
            static EngineInstance& GetInstance() {
                static EngineInstance instance; // guaranteed to be lazy-initialized and thread-safe in C++11+
                return instance;
            }

            // Public interface
            bool shouldEnd() { return platform->GetWindow()->ShouldClose(); }
            void Destroy();
            bool Run();
            void Init(EngineCreationSettings settings);

            // Optional: configure engine before initialization
            void SetSettings(const EngineCreationSettings& s) { settings = s; }
            EngineCreationSettings GetSettings() const { return settings; }

            Platform::IWindow* GetWindow() const {
                return platform->GetWindow(); }

            Platform::IInput* GetInputManager() const { return platform->GetInput(); }

        private:
            // Private constructor to prevent instantiation from outside
            EngineInstance() = default;

            EngineCreationSettings settings;
            Platform::IPlatform* platform = nullptr;
    };

    #if defined(BUILD_ENGINE)

        // Used by the EXE/engine
        inline EngineInstance& GetEngine() {
            return EngineInstance::GetInstance();
        }

    #elif defined(BUILD_GAME)

        // Used by the Game Module DLL
        inline EngineInstance* gSharedEnginePtr = nullptr;

        inline void SetEngine(EngineInstance* ptr) {
            gSharedEnginePtr = ptr;
        }

        inline EngineInstance& GetEngine() {
            if (!gSharedEnginePtr)
                exit(2);
            return *gSharedEnginePtr;
        }

    #elif defined(BUILD_EDITOR)

        // Used by the Editor Module DLL
        inline EngineInstance* gSharedEnginePtr = nullptr;

        inline void SetEngine(EngineInstance* ptr) {
            gSharedEnginePtr = ptr;
        }

        inline EngineInstance& GetEngine() {
            if (!gSharedEnginePtr)
                exit(2);
            return *gSharedEnginePtr;
        }

    #endif
}