#include "engine/core/engine.hpp"
#include "engine/core/resources/resources_manager.hpp"
#include "engine/rendering/ui/text.hpp"
#include "editor/core/platform/qt/qt_platform.hpp"
#include "engine/rendering/opengl/opengl.hpp"

using namespace Epoch::Engine;
using namespace Epoch::Engine::Core;
using namespace Epoch::Engine::Rendering;
using namespace Epoch::Engine::ECS::Components;
using namespace Epoch::Engine::ECS::Objects;


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

extern "C" __declspec(dllexport) void EditorStart(){
    
}

extern "C" __declspec(dllexport) void EditorTick(){

}

extern "C" __declspec(dllexport) void EditorCleanup(){

}

extern "C" __declspec(dllexport) Platform::IPlatform* CreatePlatform() {
    return new Platform::QTPlatform();
}