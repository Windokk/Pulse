#include "engine/core/engine.hpp"
#include "engine/serialization/level/level_serializer.hpp"
#include "engine/core/resources/resources_manager.hpp"
#include "module_loader.hpp"

using namespace Epoch::Engine;
using namespace Epoch::Engine::Core;
using namespace Epoch::Engine::Rendering;
using namespace Epoch::Engine::Input;
using namespace Epoch::Engine::ECS::Components;
using namespace Epoch::Engine::ECS::Objects;
using namespace Epoch::Launcher;

#include <iostream>

Debugging::Level minDebugLevel = Debugging::Level::Log;
bool isEditor = false;

EngineCreationSettings ComputeEngineSettings(int argc, char* argv[]) {
    Core::EngineCreationSettings settings;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--width") == 0 && i + 1 < argc) {
            settings.windowWidth = std::stoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--height") == 0 && i + 1 < argc) {
            settings.windowHeight = std::stoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--fullscreen") == 0) {
            settings.fullscreen = true;
        }
        else if (strcmp(argv[i], "--no-vsync") == 0) {
            settings.vsync = false;
        }
        else if (strcmp(argv[i], "--fps") == 0 && i + 1 < argc) {
            settings.targetFPS = std::stoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--root") == 0 && i + 1 < argc) {
            settings.rootPath = argv[++i];
        }
        else if (strcmp(argv[i], "--gravity") == 0 && i + 3 < argc) {
            float x = std::stof(argv[++i]);
            float y = std::stof(argv[++i]);
            float z = std::stof(argv[++i]);
            settings.gravity = glm::vec3(x, y, z);
        }
        else if (strcmp(argv[i], "--debug") == 0 && i + 1 < argc) {
            std::string level = argv[++i];
            if (level == "log") {
                minDebugLevel = Debugging::Level::Log;
            } else if (level == "info") {
                minDebugLevel = Debugging::Level::Info;
            } else if (level == "warning") {
                minDebugLevel = Debugging::Level::Warning;
            } else if (level == "error") {
                minDebugLevel = Debugging::Level::Error;
            } else if (level == "fatal") {
                minDebugLevel = Debugging::Level::Fatal;
            } else {
                std::cerr << "Unknown debug level: " << level << ". Using default (Log).\n";
            }
        }
        else if(strcmp(argv[i], "--editor") == 0){
            isEditor = true;
        }
    }

    return settings;
}

int main(int argc, char *argv[]) {

    EngineCreationSettings engineSettings = ComputeEngineSettings(argc, argv);

    std::string editorLib =
        #if defined(_WIN32)
            "libEditorModule.dll";
        #else
            "libEditorModule.so";
        #endif

    std::string gameLib =
        #if defined(_WIN32)
            "libGameModule.dll";
        #else
            "libGameModule.so";
        #endif

    auto& loader = ModuleLoader::GetInstance();


    if(isEditor){
        if (!loader.LoadModule("editor", editorLib)) {
            return 1;
        }

        auto initEditor = loader.GetSymbol<EditorInitFn>("editor", "InitializeSingletons");
        initEditor(&EngineInstance::GetInstance(), &Debugging::Debugger::GetInstance(), &Renderer::GetInstance(), &Resources::ResourcesManager::GetInstance(), &CameraManager::GetInstance(), &Time::TimeManager::GetInstance(), &GetGL());
    
        auto startEditor = loader.GetSymbol<EditorStartFn>("editor", "EditorStart");
        startEditor(argc, argv);

        auto createPlatform = loader.GetSymbol<CreatePlatformFn>("editor", "CreatePlatform");
        engineSettings.platform = createPlatform();
    }
    else{
        if (!loader.LoadModule("game", gameLib)) {
            return 1;
        }

        auto initGame = loader.GetSymbol<GameInitFn>("game", "InitializeSingletons");
        initGame(&EngineInstance::GetInstance(), &Debugging::Debugger::GetInstance(), minDebugLevel, &ECS::Components::GetComponentRegistry(), &GetGL());
    
        auto registerGameComponents = loader.GetSymbol<GameRegisterComponentsFn>("game", "RegisterGameComponents");
        registerGameComponents();

        auto createPlatform = loader.GetSymbol<CreatePlatformFn>("game", "CreatePlatform");
        engineSettings.platform = createPlatform();
    }


    EngineInstance::GetInstance().Init(engineSettings); // Safe initialization of Engine Instance

    std::shared_ptr<Epoch::Engine::Levels::Level> level = Core::Resources::ResourcesManager::GetInstance().GetLevel("sponza.lvl");
    //Levels::LevelManager::GetInstance().LoadLevel(level);

    std::shared_ptr<Rendering::Shader> fbShader = Core::Resources::ResourcesManager::GetInstance().GetShader("shaders\\fb\\framebuffer");
    Rendering::FrameBuffer sceneFB = {Core::GetEngine().GetWindow()->GetFramebufferWidth(), Core::GetEngine().GetWindow()->GetFramebufferHeight(), fbShader, true};
    std::function<void()> drawFunc = []() {
        Rendering::Renderer::GetInstance().DrawScene();
    };
    Rendering::Renderer::GetInstance().AddRenderPass(Rendering::RenderStage::Scene, drawFunc, std::make_shared<Rendering::FrameBuffer>(sceneFB), true, Rendering::BlendMode::Normal);

    while (!EngineInstance::GetInstance().shouldEnd())
    {
        if(!EngineInstance::GetInstance().Run()){
            break;
        };
        
        if(isEditor){
            auto tickEditor = loader.GetSymbol<EditorTickFn>("editor", "EditorTick");
            tickEditor();
        }
    }

    if(isEditor){
        auto cleanupEditor = loader.GetSymbol<EditorCleanupFn>("editor", "EditorCleanup");
        cleanupEditor();
    }

    EngineInstance::GetInstance().Destroy();
    std::cout << "Epoch Engine has finished. Press Enter to exit..." << std::endl;
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();

    return 0;

}