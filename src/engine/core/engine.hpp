#pragma once

#include <stdexcept>

#include "engine/filesystem/filesystem.hpp"
#include "engine/levels/level_manager.hpp"
#include "engine/ecs/objects/actors/actor.hpp"
#include "engine/core/platform/iplatform.hpp"
#include "engine/time/time_manager.hpp"
#include "engine/core/resources/resources_manager.hpp"
#include "engine/rendering/renderer/renderer.hpp"
#include "engine/projects/project.hpp"


namespace Epoch::Engine::Rendering {
    
    class CameraManager;
}

namespace Epoch::Engine::Core {
    
    class ResourcesManager;

    struct EngineCreationSettings{
        //PLATFORM
        Platform::IPlatform* platform = nullptr;

        //PROJECT
        std::string project = "";

        //WINDOW (Editor preferences)
        int windowWidth = 1280;
        int windowHeight = 720;
        bool fullscreen = false;
        bool vsync = true;

        //PHYSICS (project settings : physics)
        glm::vec3 gravity = glm::vec3(0, -9.81f, 0);
    };

    struct EngineContext {
        Rendering::Renderer* renderer = nullptr;
        Rendering::CameraManager* cameraManager = nullptr;
        OpenGL* openGL = nullptr;

        Resources::ResourcesManager* resourcesManager = nullptr;
        Filesystem::FileManager* fileManager = nullptr;
        Filesystem::AssetIDManager* assetIDManager = nullptr;

        ECS::ObjectIDManager* objIDManager = nullptr;

        Levels::LevelManager* levelManager = nullptr;

        Events::EventDispatcher* eventDispatcher = nullptr;

        Audio::AudioManager* audioManager = nullptr;

        Audio::AudioIDManager* audioIDManager = nullptr;

        Physics::PhysicsSystem* physicsSystem = nullptr;

        Time::TimeManager* timeManager = nullptr;

        Debugging::Debugger* debugger = nullptr;

        Platform::IPlatform* platform = nullptr;

        std::shared_ptr<Projects::Project> currentProject = nullptr;
    };

    class EngineInstance {
        public:
            EngineInstance(const EngineInstance&) = delete;
            EngineInstance& operator=(const EngineInstance&) = delete;

            // Provide access to the singleton instance
            static EngineInstance& GetInstance() {
                static EngineInstance instance;
                return instance;
            }

            // Public interface
            bool shouldEnd() { return context.platform->GetWindow()->ShouldClose(); }
            void Destroy();
            bool Run();
            void Init(EngineCreationSettings settings);
            void InitSystems();

            // Optional: configure engine before initialization
            void SetSettings(const EngineCreationSettings& s) { settings = s; }
            EngineCreationSettings GetSettings() const { return settings; }

            Platform::IWindow* GetWindow() const { return context.platform->GetWindow(); }

            Platform::IInput* GetInputManager() const { return context.platform->GetInput(); }

            Rendering::Renderer* GetRenderer() const { return context.renderer; }

            Rendering::CameraManager* GetCameraManager() const { return context.cameraManager; }

            OpenGL* GetGL() const { return context.openGL; }

            void SetGL(OpenGL* gl) { context.openGL = gl; }

            Resources::ResourcesManager* GetResourcesManager() const { return context.resourcesManager; }

            Filesystem::FileManager* GetFileManager() const { return context.fileManager; }

            Filesystem::AssetIDManager* GetAssetIDManager() const { return context.assetIDManager; }

            ECS::ObjectIDManager* GetObjectIDManager() const { return context.objIDManager; }

            Physics::PhysicsSystem* GetPhysicsSystem() const { return context.physicsSystem; }

            Levels::LevelManager* GetLevelManager() const { return context.levelManager; }

            Events::EventDispatcher* GetEventDispatcher() const { return context.eventDispatcher; }

            Audio::AudioManager* GetAudioManager() const { return context.audioManager; }

            Audio::AudioIDManager* GetAudioIDManager() const { return context.audioIDManager; }

            Time::TimeManager* GetTimeManager() const { return context.timeManager; }

            Debugging::Debugger* GetDebugger() const { return context.debugger; }

            Projects::BuildSettings* GetBuildSettings() const { return context.currentProject->GetBuildSettings(); }

            std::shared_ptr<Projects::Project> GetCurrentProject() const { return context.currentProject; }

        private:
            EngineInstance() = default;

            EngineContext context;

            EngineCreationSettings settings;
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
