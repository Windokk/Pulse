#include "engine/core/engine.hpp"
#include "engine/serialization/level/level_serializer.hpp"
#include "engine/core/resources/resources_manager.hpp"
#include "game_module_loader.hpp"

using namespace Pulse::Engine;
using namespace Pulse::Engine::Core;
using namespace Pulse::Engine::Rendering;
using namespace Pulse::Engine::Input;
using namespace Pulse::Engine::ECS::Components;
using namespace Pulse::Engine::ECS::Objects;
using namespace Pulse::Game;

#include <iostream>

Debugging::Level minDebugLevel = Debugging::Level::Log;
std::string mainModuleLib = "";

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
        else if (strcmp(argv[i], "--vsync") == 0) {
            settings.vsync = false;
        }
        else if (strcmp(argv[i], "--project") == 0 && i + 1 < argc) {
            settings.project = argv[++i];
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
        else if(strcmp(argv[i], "--game") == 0 && i + 1 < argc){
            mainModuleLib = argv[++i];
        }
    }

    return settings;
}

void early_crash(){
    std::cout << "Pulse Engine has crashed. Press Enter to exit..." << std::endl;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
    std::terminate();
}

int main(int argc, char* argv[]) {

    // Engine init parameters
    EngineCreationSettings engineSettings = ComputeEngineSettings(argc, argv);

    if(engineSettings.project == ""){
        std::cerr<<"No project specified, aborting..."<<std::endl;
        early_crash();
    }

    //Module loader init
    auto& loader = ModuleLoader::GetInstance();
    const std::string mainModuleName = "game";

    //Game module loading
    if (!loader.LoadModule(mainModuleName, mainModuleLib)) {
        std::cerr << "Failed to load module: editor" << std::endl;
        early_crash();
    }

    //Game module init
    {
        auto initGame = loader.GetSymbol<GameInitFn>("game", "InitializeSingletons");
        if (!initGame){
            std::cerr<<"Failed to find symbol: InitializeSingletons"<<std::endl;
            early_crash();
        }

        initGame(&Core::GetEngine(),
                 &ECS::Components::GetComponentRegistry());

        auto registerGameComponents = loader.GetSymbol<GameRegisterComponentsFn>("game", "RegisterGameComponents");
        if (!registerGameComponents){
            std::cerr<<"Failed to find symbol: RegisterGameComponents"<<std::endl;
            early_crash();
        }
        registerGameComponents();
    }

    // Platform creation
    auto createPlatform = loader.GetSymbol<CreatePlatformFn>("game", "CreatePlatform");
    if (!createPlatform) {
        std::cerr << "Failed to find symbol: CreatePlatform" << std::endl;
        early_crash();
    }
    engineSettings.platform = createPlatform(argc, argv);

    // Engine startup
    Core::GetEngine().Init(engineSettings);

    {
        // Scene framebuffer
        auto fbShader = Core::GetEngine().GetResourcesManager()->GetShader("shaders\\fb\\framebuffer");
        Rendering::FrameBuffer sceneFB(
            Core::GetEngine().GetWindow()->GetFramebufferWidth(),
            Core::GetEngine().GetWindow()->GetFramebufferHeight(),
            fbShader,
            true
        );

        // Main renderpasss
        auto drawFunc = []() {
            Core::GetEngine().GetRenderer()->DrawScene();
        };

        Core::GetEngine().GetRenderer()->AddRenderPass(
            Rendering::RenderStage::Scene,
            drawFunc,
            std::make_shared<Rendering::FrameBuffer>(sceneFB),
            true,
            Rendering::BlendMode::Normal
        );
    }

    //Main Loop
    while (!Core::GetEngine().shouldEnd()) {
        if (!Core::GetEngine().Run()) break;
    }

    //Cleaning
    Core::GetEngine().Destroy();

    std::cout << "Pulse Engine has finished. Press Enter to exit..." << std::endl;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();

    return 0;
}