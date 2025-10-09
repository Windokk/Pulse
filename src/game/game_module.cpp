#include "engine/ecs/components/core/registry/component_registry.hpp"
#include "engine/ecs/components/core/registry/dll_component_registration.hpp"
#include "engine/core/engine.hpp"
#include "game/core/platform/glfw/glfw_platform.hpp"
#include "engine/rendering/opengl/opengl.hpp"

using namespace Epoch::Engine;
using namespace Epoch::Engine::Core;
using namespace Epoch::Engine::ECS::Components;

extern "C" __declspec(dllexport) void InitializeSingletons(Core::EngineInstance* engine, Debugging::Debugger* debugger, 
                                                            Debugging::Level minDebugLevel, ComponentRegistry* compReg,
                                                            OpenGL* openGLBindings) {
    SetEngine(engine);
    SetDebugger(debugger);
    Debugging::GetDebugger().SetMinimumLevel(minDebugLevel);
    SetComponentRegistry(compReg);
    SetGL(openGLBindings);
}

extern "C" __declspec(dllexport) void RegisterGameComponents() {
    for (auto& cb : GetComponentRegistrars()) {
        cb(GetComponentRegistry());
    }
}

extern "C" __declspec(dllexport) Platform::IPlatform* CreatePlatform() {
    return new Platform::GLFWPlatform();
}
