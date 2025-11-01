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


namespace Pulse::Engine::Rendering {
    
    class CameraManager;
}

namespace Pulse::Engine::Core {
    
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

    
    struct Stats {
        //Audio
        int sounds = 0;

        //Rendering
        float frameTimeMs = 0;
        float fps = 0;
        int drawCalls = 0;
        int triangles = 0;
        int vertices = 0;
        float gpuMemoryMB = 0;

        //Level
        int actors = 0;
        int lights = 0;
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

            Stats GetStats() {
                Stats ret{};

                for(auto& cmd : *context.renderer->GetDrawList()){
                    ret.drawCalls++;
                    ret.triangles += cmd.indexCount / 3;
                    ret.vertices += cmd.verticesCount;
                }

                ret.frameTimeMs = context.timeManager->GetDeltaTime() * 1000;
                ret.fps = 1000 / ret.frameTimeMs;

                ret.actors = static_cast<int>(context.levelManager->GetLevelAt(0)->transforms.size());
                ret.lights = context.renderer->lightMan->GetLightsCount();

                ret.sounds = context.audioManager->GetSoundsCount();

                Platform::SystemInfos infos = GetWindow()->GetSystemInfos();

                if(infos.gpu_vendor == "NVIDIA Corporation"){    
                    GLint total_kb = 0;
                    GLint available_kb = 0;

                    context.openGL->GetIntegerv(0x9048, &total_kb);
                    context.openGL->GetIntegerv(0x9049, &available_kb);

                    ret.gpuMemoryMB = (total_kb - available_kb) / 1024.0f;
                }
                else if(infos.gpu_vendor == "ATI Technologies Inc."){
                    GLint freeMemoryKB[4] = {0};
                    context.openGL->GetIntegerv(0x87FC, freeMemoryKB);

                    ret.gpuMemoryMB = (float)(freeMemoryKB[0]) / 1024.0f;
                }
                else{
                    ret.gpuMemoryMB += (GetWindow()->GetFramebufferWidth() * GetWindow()->GetFramebufferHeight() * GetWindow()->GetBytesPerPixel()) / (1024.0f * 1024.0f);
                    ret.gpuMemoryMB += (ret.vertices * sizeof(Vertex)) / (1024.0f * 1024.0f);
                    ret.gpuMemoryMB += (ret.triangles * 3 * sizeof(uint32_t)) / (1024.0f * 1024.0f);
                }

                return ret;
            }

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
