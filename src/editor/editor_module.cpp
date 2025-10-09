#include "engine/core/engine.hpp"
#include "engine/core/resources/resources_manager.hpp"
#include "engine/rendering/ui/text.hpp"
#include "editor/core/platform/qt/qt_platform.hpp"
#include "engine/rendering/opengl/opengl.hpp"

#include <QApplication>

using namespace Epoch::Engine;
using namespace Epoch::Engine::Core;
using namespace Epoch::Engine::Rendering;
using namespace Epoch::Engine::ECS::Components;
using namespace Epoch::Engine::ECS::Objects;

static QApplication* s_app = nullptr;

extern "C" __declspec(dllexport) void InitializeSingletons(Core::EngineInstance* engine, Debugging::Debugger* debugger, 
                                                            Renderer* renderer, Core::Resources::ResourcesManager* resourcesManager, 
                                                            CameraManager* cameraManager, Time::TimeManager* timeManager,
                                                            OpenGL* openGLBindings) {
    SetEngine(engine);                                                           
    SetDebugger(debugger);
    SetRenderer(renderer);
    SetResourcesManager(resourcesManager);
    SetCameraManager(cameraManager);
    SetTimeManager(timeManager);
    SetGL(openGLBindings);
}

extern "C" __declspec(dllexport) void EditorStart(int argc, char** argv){
    if (!s_app) {
        s_app = new QApplication(argc, argv);
    }
}

extern "C" __declspec(dllexport) void EditorTick(){

}

extern "C" __declspec(dllexport) void EditorCleanup(){

}

extern "C" __declspec(dllexport) Platform::IPlatform* CreatePlatform() {
    return new Platform::QTPlatform();
}