#include "engine.hpp"

#include <iostream>
#include <string>
#include <thread>

#include "engine/core/resources/resources_manager.hpp"

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
        if(settings.platform == nullptr)
            DEBUG_FATAL("Platform settings are nullptr !");

        this->settings = settings;
        this->platform = settings.platform;
        platform->CreateWindow("Epoch", settings.windowWidth, settings.windowHeight, settings.fullscreen, settings.vsync);

        FileManager::Init(settings.rootPath);
        PhysicsSystem::Init(settings.gravity);
        AudioManager::GetInstance().Init(100.0f);
        Renderer::GetInstance().Init();
        Resources::ResourcesManager::GetInstance().LoadResources(Filesystem::Path("project_resources"), Filesystem::Path("engine_resources"));
        Renderer::GetInstance().InitFramebuffers();
        platform->CreateInput();
        
        EventDispatcher::GetInstance();

        float fixedDelta = 1.0f / 30.0f;

        Time::TimeManager::GetInstance().Init(fixedDelta);
    }

    void EngineInstance::Destroy()
    {
        platform->GetInput()->Shutdown();
        AudioManager::GetInstance().Shutdown();
        PhysicsSystem::Shutdown();
        LevelManager::GetInstance().UnloadAllLevels();
        Renderer::GetInstance().Shutdown();
        platform->GetWindow()->Destroy();
    }

    bool EngineInstance::Run() {
        auto& time = Time::TimeManager::GetInstance();
        time.Tick();

        //PhysicsSystem::StepSimulation(time.GetFixedDeltaTime());

        platform->GetWindow()->PollEvents();

        Renderer::GetInstance().Render();
        AudioManager::GetInstance().Tick();
        platform->GetInput()->Tick();

        LevelManager::GetInstance().Tick();

        platform->GetWindow()->SwapBuffers();

        if(platform->GetInput()->WasKeyPressed(Key::Escape))
        {
            return false;
        }

        return true;
    }

}