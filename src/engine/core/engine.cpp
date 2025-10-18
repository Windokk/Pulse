#include "engine.hpp"

#include <iostream>
#include <string>
#include <thread>

#include "engine/core/resources/resources_manager.hpp"

#include "engine/debugging/debugger.hpp"

#include "engine/serialization/project/project_serializer.hpp"

using namespace std::chrono;

namespace Epoch::Engine::Core{
    
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
        context.platform->CreateWindow("Epoch", settings.windowWidth, settings.windowHeight, settings.fullscreen, settings.vsync);

        context.currentProject = Serialization::DeserializeProject(Filesystem::Path(settings.project, true));

        context.fileManager->Init(context.currentProject->GetProjectResourcesPath(), context.fileManager->GetCurrentExecutablePath() / "engine_resources", context.currentProject->GetProjectRoot());
        
        context.resourcesManager->ConstructGlobalFileIndex(context.currentProject->GetProjectResourcesPath());

        context.physicsSystem->Init(settings.gravity);
        context.audioManager->Init(100.0f);
        context.renderer->Init();
        context.renderer->InitFramebuffers();
        context.platform->CreateInput();
        
        context.eventDispatcher = new EventDispatcher();

        float fixedDelta = 1.0f / 30.0f;

        context.timeManager->Init(fixedDelta);

        if(context.currentProject->GetBuildSettings()->buildIndex.size() > 0){
            Filesystem::Path defaultLevelPathAbs = context.currentProject->GetBuildSettings()->buildIndex[0];
            DEBUG_LOG("Loading default level : "+defaultLevelPathAbs.full);
            std::string defaultLevelPath = defaultLevelPathAbs.RelativeTo(context.currentProject->GetProjectResourcesPath()).full;
            auto level = GetResourcesManager()->GetLevel(defaultLevelPath);
            GetLevelManager()->LoadLevel(level);
        }
        
    }

    void EngineInstance::InitSystems()
    {
        context.debugger = new Debugging::Debugger();
     
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
    }

    void EngineInstance::Destroy()
    {
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