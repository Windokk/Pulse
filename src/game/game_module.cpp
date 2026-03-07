#include "engine/ecs/components/core/registry/component_registration.hpp"
#include "engine/core/engine.hpp"

#include "character.reflection.hpp"

using namespace Pulse::Engine;
using namespace Pulse::Engine::Core;
using namespace Pulse::Engine::ECS::Components;
using namespace Pulse::Engine::Debugging;

#if defined(_WIN32) || defined(_WIN64)
#   define API_EXPORT __declspec(dllexport)
#else
#   define API_EXPORT __attribute__((visibility("default")))
#endif

extern "C" API_EXPORT void InitializeSingletons(Core::EngineInstance* engine, 
                                                            ComponentRegistry* compReg, Logger* logger) {
    SetEngine(engine);
    SetComponentRegistry(compReg);
    SetLogger(logger);
}

extern "C" API_EXPORT void RegisterGameComponents() {
    for (auto& cb : GetComponentRegistrars()) {
        cb(GetComponentRegistry());
    }
}
