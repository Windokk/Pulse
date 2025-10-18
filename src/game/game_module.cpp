#include "engine/ecs/components/core/registry/component_registry.hpp"
#include "engine/ecs/components/core/registry/dll_component_registration.hpp"
#include "engine/core/engine.hpp"
#include "game/core/platform/glfw/glfw_platform.hpp"
#include "engine/rendering/opengl/opengl.hpp"

using namespace Epoch::Engine;
using namespace Epoch::Engine::Core;
using namespace Epoch::Engine::ECS::Components;

extern "C" __declspec(dllexport) void InitializeSingletons(Core::EngineInstance* engine, 
                                                            ComponentRegistry* compReg) {
    SetEngine(engine);
    SetComponentRegistry(compReg);
}

extern "C" __declspec(dllexport) void RegisterGameComponents() {
    for (auto& cb : GetComponentRegistrars()) {
        cb(GetComponentRegistry());
    }
}

extern "C" __declspec(dllexport) Platform::IPlatform* CreatePlatform(int argc, char** argv) {
    return new Epoch::Game::Core::Platform::GLFWPlatform();
}
