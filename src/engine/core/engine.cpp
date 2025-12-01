#include "engine.hpp"

#include <iostream>
#include <string>
#include <thread>

#include "engine/core/resources/resources_manager.hpp"

#include "engine/debugging/logger.hpp"

#include "engine/serialization/project/project_serializer.hpp"

using namespace std::chrono;

namespace Pulse::Engine{

    namespace Debugging {
        Logger* g_Logger = nullptr;
    }

    namespace Core{
        
        using namespace Levels;
        using namespace Engine::Rendering;
        using namespace Physics;
        using namespace Filesystem;
        using namespace Audio;
        using namespace Input;
        using namespace Events;

        void EngineInstance::Init(EngineCreationSettings settings){

            InitSystems();

            if(settings.platform == nullptr)
                DEBUG_FATAL("Platform settings are nullptr !");

            this->settings = settings;
            this->context.platform = settings.platform;
            context.currentProject = Serialization::DeserializeProject(Filesystem::Path(settings.project, true));

            context.fileManager->Init(context.currentProject->GetProjectResourcesPath(), context.fileManager->GetCurrentExecutablePath() / "engine_resources", context.currentProject->GetProjectRoot());
            
            context.resourcesManager->ConstructGlobalFileIndex(context.currentProject->GetProjectResourcesPath());

            context.platform->CreateWindow("Pulse", settings.windowWidth, settings.windowHeight, settings.fullscreen, settings.vsync);
            
            context.physicsSystem->Init(settings.gravity);
            context.audioManager->Init(100.0f);
            context.renderer->Init();
            context.renderer->InitFramebuffers();
            context.platform->CreateInput();

            float fixedDelta = 1.0f / 30.0f;

            context.timeManager->Init(fixedDelta);

            Platform::SystemInfos infos = GetWindow()->GetSystemInfos();

            DEBUG_INFO("===== System infos =====");
            DEBUG_INFO("GPU Vendor : " + infos.gpu_vendor);
            DEBUG_INFO("GPU Renderer : " + infos.gpu_renderer);
            DEBUG_INFO("OpenGL Version : " + infos.gl_version);
            DEBUG_INFO("Using Host : " + std::string(infos.windowHost == Platform::WindowHost::QT ? "QT" : "GLFW") + " with version : " + infos.windowHostVersion);

            DEBUG_INFO("Monitors : ");
            for(int i = 0; i < infos.connectedMonitorsCount; i++){
                DEBUG_INFO("    Monitor : "+ std::to_string(i) + " : width = " + std::to_string(infos.monitors[i].width) + " px, height = "+ std::to_string(infos.monitors[i].height) + " px, refreshRate = "+std::to_string(infos.monitors[i].refreshRate)+" hz");
            }


            if(context.currentProject->GetBuildSettings()->buildIndex.size() > 0){
                Filesystem::Path defaultLevelPath = context.currentProject->GetBuildSettings()->buildIndex[0];
                DEBUG_LOG("Loading default level : "+defaultLevelPath.full);
                auto level = GetResourcesManager()->GetLevel(defaultLevelPath.full);
                if(level)
                    GetLevelManager()->LoadLevel(level);
                else
                    DEBUG_ERROR("Error loading default level !");
            }
            
        }

        void EngineInstance::InitSystems()
        {
            context.logger = new Debugging::Logger();
            Pulse::Engine::Debugging::g_Logger = context.logger;

            context.renderer = new Rendering::Renderer();
            context.cameraManager = new Rendering::CameraManager();

            context.resourcesManager = new Resources::ResourcesManager();
            context.fileManager = new Filesystem::FileManager();
            context.assetIDManager = new Filesystem::AssetIDManager();

            context.objIDManager = new ECS::ObjectIDManager();

            context.levelManager = new Levels::LevelManager();

            context.eventDispatcher = new Events::EventDispatcher();

            context.audioManager = new Audio::AudioManager();
            context.audioIDManager = new Audio::AudioIDManager();

            context.physicsSystem = new Physics::PhysicsSystem();

            context.timeManager = new Time::TimeManager();

            context.profiler = new Debugging::Profiler();
        }

        void EngineInstance::Destroy()
        {
            context.profiler->Shutdown();
            context.platform->GetInput()->Shutdown();
            context.audioManager->Shutdown();
            context.physicsSystem->Shutdown();
            context.levelManager->UnloadAllLevels();
            context.renderer->Shutdown();
            context.platform->GetWindow()->Destroy();
        }

        bool EngineInstance::Run() {

            context.timeManager->Tick();

            //PhysicsSystem::StepSimulation(time.GetFixedDeltaTime());

            context.renderer->Render();
            context.audioManager->Tick();
            context.platform->GetInput()->Tick();

            context.levelManager->Tick();

            context.platform->GetWindow()->PollEvents();
            context.platform->GetWindow()->SwapBuffers();

            if(context.platform->GetInput()->WasKeyPressed(Key::Escape))
            {
                return false;
            }

            return true;
        }

    }
}