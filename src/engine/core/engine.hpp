#pragma once

#include <stdexcept>
#include <memory>
#include <glm/glm.hpp>

namespace Pulse::Engine{

    namespace Rendering {
        class Renderer;
        class CameraManager;
        struct RendererSettings;
    }

    namespace Events {
        class EventDispatcher;
    }

    namespace Levels{
        class LevelManager;
    }

    namespace Audio{
        class AudioManager;
        class AudioIDManager;
    }

    namespace Physics{
        class PhysicsManager;
    }

    namespace Time{
        class TimeManager;
    }

    namespace Projects{
        class Project;
    }

    namespace Debugging{
        class Profiler;
    }

    namespace Projects{
        struct BuildSettings;
    }

    namespace Filesystem{
        class FileManager;
        class AssetIDManager;
    }

    namespace Core {

        namespace Resources{
            class ResourcesManager;
        }

        namespace Platform{
            class IPlatform;
            class IInput;
            class IWindow;
        }

        class ObjectIDManager;

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

            //PLAY MODE
            bool startInPlayMode = false;

            uint32_t api;
        };

        struct EngineContext {

            Rendering::Renderer* renderer = nullptr;
            Rendering::CameraManager* cameraManager = nullptr;

            Resources::ResourcesManager* resourcesManager = nullptr;
            Filesystem::FileManager* fileManager = nullptr;
            Filesystem::AssetIDManager* assetIDManager = nullptr;

            ObjectIDManager* objIDManager = nullptr;

            Levels::LevelManager* levelManager = nullptr;

            Events::EventDispatcher* eventDispatcher = nullptr;

            Audio::AudioManager* audioManager = nullptr;

            Audio::AudioIDManager* audioIDManager = nullptr;

            Physics::PhysicsManager* physicsManager = nullptr;

            Time::TimeManager* timeManager = nullptr;

            Platform::IPlatform* platform = nullptr;

            std::shared_ptr<Projects::Project> currentProject = nullptr;

            Debugging::Profiler* profiler = nullptr;
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
                bool ShouldEnd();
                void Destroy();
                bool Run();
                void Init(EngineCreationSettings settings);
                void InitSystems();

                // Optional: configure engine before initialization
                void SetSettings(const EngineCreationSettings& s) { m_EngineSettings = s; }
                EngineCreationSettings GetSettings() const { return m_EngineSettings; }

                Platform::IWindow* GetWindow() const;

                Platform::IInput* GetInputManager() const;

                Rendering::Renderer* GetRenderer() const { return m_Context.renderer; }

                Rendering::CameraManager* GetCameraManager() const { return m_Context.cameraManager; }

                Resources::ResourcesManager* GetResourcesManager() const { return m_Context.resourcesManager; }

                Filesystem::FileManager* GetFileManager() const { return m_Context.fileManager; }

                Filesystem::AssetIDManager* GetAssetIDManager() const { return m_Context.assetIDManager; }

                ObjectIDManager* GetObjectIDManager() const { return m_Context.objIDManager; }

                Physics::PhysicsManager* GetPhysicsManager() const { return m_Context.physicsManager; }

                Levels::LevelManager* GetLevelManager() const { return m_Context.levelManager; }

                Events::EventDispatcher* GetEventDispatcher() const { return m_Context.eventDispatcher; }

                Audio::AudioManager* GetAudioManager() const { return m_Context.audioManager; }

                Audio::AudioIDManager* GetAudioIDManager() const { return m_Context.audioIDManager; }

                Time::TimeManager* GetTimeManager() const { return m_Context.timeManager; }

                Projects::BuildSettings* GetBuildSettings() const;

                std::shared_ptr<Projects::Project> GetCurrentProject() const { return m_Context.currentProject; }

                Debugging::Profiler* GetProfiler() const { return m_Context.profiler; }

                bool IsInPlayMode() const { return m_PlayMode; }

                void SetPlayMode(bool on);

            private:
                EngineInstance() = default;

                EngineContext m_Context;

                EngineCreationSettings m_EngineSettings;

                bool m_PlayMode = false;
                bool m_ActivateAllPhysics = false;
                bool m_ReloadCurrentLevel = false;
                
                std::shared_ptr<Rendering::RendererSettings> m_RendererSettings;
        };

        #if defined(BUILD_ENGINE)

            // Used by the EXE/engine
            inline EngineInstance* gSharedEnginePtr = nullptr;

            inline void SetEngine(EngineInstance* ptr) {
                gSharedEnginePtr = ptr;
            }

            inline EngineInstance& GetEngine() {
                if (!gSharedEnginePtr)
                    exit(2);
                return *gSharedEnginePtr;
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
        #endif
    }
   
}
