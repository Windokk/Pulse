#include "engine.hpp"

#include <iostream>
#include <string>
#include <thread>

#include "engine/core/resources/resources_manager.hpp"
#include "engine/debugging/logger.hpp"
#include "engine/serialization/project/project_serializer.hpp"
#include "engine/levels/level_manager.hpp"
#include "engine/debugging/profiler.hpp"
#include "engine/core/platform/iplatform.hpp"
#include "engine/time/time_manager.hpp"
#include "engine/rendering/camera/camera_manager.hpp"
#include "engine/ecs/objects/actors/actor.hpp"
#include "engine/audio/audio_manager.hpp"

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

            m_EngineSettings = settings;
            m_Context.platform = settings.platform;
            std::shared_ptr<Pulse::Engine::Projects::Project> project = Serialization::DeserializeProject(Filesystem::Path(settings.project, true));
            if(!project){
                DEBUG_FATAL("Project is nullptr, aborting...");
            }
            else{
                m_Context.currentProject = project;
            }

            m_Context.fileManager->Init(m_Context.currentProject->GetProjectResourcesPath(), m_Context.fileManager->GetCurrentExecutablePath() / "engine_resources", m_Context.currentProject->GetProjectRoot());
            
            m_Context.resourcesManager->ConstructGlobalFileIndex(m_Context.currentProject->GetProjectResourcesPath());

            m_Context.platform->CreateWindow("Pulse", settings.windowWidth, settings.windowHeight, settings.fullscreen, settings.vsync, settings.api);
            
            m_Context.physicsManager->Init(settings.gravity);
            m_Context.audioManager->Init(100.0f);
            m_RendererSettings = std::make_shared<RendererSettings>();
            m_RendererSettings->viewportWidth = m_Context.platform->GetWindow()->GetFramebufferWidth();
            m_RendererSettings->viewportHeight = m_Context.platform->GetWindow()->GetFramebufferHeight();
            m_RendererSettings->api = (RendererAPI::API)settings.api;
            m_Context.renderer->Init(m_RendererSettings);
            m_Context.platform->CreateInput();

            float fixedDelta = 1.0f / 30.0f;

            m_Context.timeManager->Init(fixedDelta);

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

            if(m_Context.currentProject->GetBuildSettings()->buildIndex.size() > 0){
                Filesystem::Path defaultLevelPath = m_Context.currentProject->GetBuildSettings()->buildIndex[0];
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
            m_Context.renderer = new Rendering::Renderer();
            m_Context.cameraManager = new Rendering::CameraManager();

            m_Context.resourcesManager = new Resources::ResourcesManager();
            m_Context.fileManager = new Filesystem::FileManager();
            m_Context.assetIDManager = new Filesystem::AssetIDManager();

            m_Context.objIDManager = new ObjectIDManager();

            m_Context.levelManager = new Levels::LevelManager();

            m_Context.eventDispatcher = new Events::EventDispatcher();

            m_Context.audioManager = new Audio::AudioManager();
            m_Context.audioIDManager = new Audio::AudioIDManager();

            m_Context.physicsManager = new Physics::PhysicsManager();

            m_Context.timeManager = new Time::TimeManager();

            m_Context.profiler = new Debugging::Profiler();
        }

        Platform::IWindow *EngineInstance::GetWindow() const
        {
            return m_Context.platform->GetWindow();
        }

        Platform::IInput *EngineInstance::GetInputManager() const
        {
            return m_Context.platform->GetInput();
        }

        Projects::BuildSettings *EngineInstance::GetBuildSettings() const
        {
            return m_Context.currentProject ? m_Context.currentProject->GetBuildSettings() : nullptr;
        }

        void EngineInstance::SetPlayMode(bool on)
        {
            m_PlayMode = on;

            if(m_PlayMode){

                m_Context.levelManager->GetLevelAt(0)->Serialize(m_Context.levelManager->GetLevelAt(0)->GetPath());

                for(int i = 0; i < m_Context.levelManager->GetLoadedLevelCount(); i++){
                    m_Context.levelManager->GetLevelAt(i)->Play();
                }

                auto* level = m_Context.levelManager->GetLevelAt(0);
                if (level && !level->cameras.empty()){

                    auto cameraEntry = level->cameras.begin();
                    auto camera = cameraEntry->second;
                    if (!camera) {
                        DEBUG_ERROR("First camera is not valid for level : ", level->GetName());
                        return;
                    }

                    auto parent = camera->parent;
                    if (!parent) {
                        DEBUG_ERROR("First camera's parent is not valid for level : ", level->GetName());
                        return;
                    }

                    m_Context.cameraManager->SetActiveCamera(parent->GetID());
                }

                m_ActivateAllPhysics = true;
            }
            else{
                
                for(int i = 0; i < m_Context.levelManager->GetLoadedLevelCount(); i++){
                    m_Context.levelManager->GetLevelAt(i)->Stop();
                }

                m_ReloadCurrentLevel = true;
            }
        }

        bool EngineInstance::ShouldEnd()
        {
            return m_Context.platform && m_Context.platform->GetWindow()->ShouldClose();
        }

        void EngineInstance::Destroy()
        {
            m_Context.currentProject->Shutdown(m_EngineSettings.project);
            m_Context.profiler->Shutdown();
            m_Context.platform->GetInput()->Shutdown();
            m_Context.audioManager->Shutdown();
            m_Context.physicsManager->Shutdown();
            m_Context.levelManager->UnloadAllLevels();
            m_Context.renderer->Shutdown();
            m_Context.platform->GetWindow()->Destroy();
        }

        bool EngineInstance::Run() {

            if(m_PlayMode){
                m_Context.physicsManager->StepSimulation(m_Context.timeManager->GetFixedDeltaTime(), m_ActivateAllPhysics);
                m_Context.audioManager->Tick();
                m_Context.levelManager->Tick();
            }

            if(m_ActivateAllPhysics)
                m_ActivateAllPhysics = false;

            if(m_ReloadCurrentLevel)
            {
                std::string levelNameInProject = GetAssetIDManager()->GetAssetFromID(GetLevelManager()->GetLevelAt(0)->GetAssetID())->baseInfos.nameInProject;
                GetLevelManager()->UnloadLevel(0);
                GetResourcesManager()->UnloadLevel(levelNameInProject);
                auto level = GetResourcesManager()->GetLevel(levelNameInProject);
                if(level)
                    GetLevelManager()->LoadLevel(level);
                else
                    DEBUG_ERROR("Error re-loading level !");
                m_ReloadCurrentLevel = false;
            }

            m_Context.physicsManager->TickBodies(m_Context.timeManager->GetFixedDeltaTime());
            m_Context.timeManager->Tick();
            m_Context.renderer->Render();
            m_Context.platform->GetInput()->Tick();
            m_Context.platform->GetWindow()->PollEvents();
            m_Context.platform->GetWindow()->SwapBuffers();

            if(m_Context.platform->GetInput()->WasKeyPressed(Key::Escape))
            {
                return false;
            }

            return true;
        }

    }
}