#include "engine/ecs/components/core/registry/component_registry.hpp"
#include "engine/ecs/components/core/registry/component_registration.hpp"
#include "engine/core/engine.hpp"
#include "game/core/platform/glfw/glfw_platform.hpp"
#include "engine/rendering/opengl/opengl.hpp"

using namespace Pulse::Engine;
using namespace Pulse::Engine::Core;
using namespace Pulse::Engine::ECS::Components;

#if defined(_WIN32) || defined(_WIN64)
#   define API_EXPORT __declspec(dllexport)
#else
#   define API_EXPORT __attribute__((visibility("default")))
#endif

extern "C" API_EXPORT void InitializeSingletons(Core::EngineInstance* engine, 
                                                            ComponentRegistry* compReg) {
    SetEngine(engine);
    SetComponentRegistry(compReg);
}

extern "C" API_EXPORT void RegisterGameComponents() {
    for (auto& cb : GetComponentRegistrars()) {
        cb(GetComponentRegistry());
    }
}

extern "C" API_EXPORT Platform::IPlatform* CreatePlatform(int argc, char** argv) {
    return new Pulse::Game::Core::Platform::GLFWPlatform();
}
