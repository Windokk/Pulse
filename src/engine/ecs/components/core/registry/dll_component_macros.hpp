#pragma once

#include "engine/ecs/components/core/component.hpp"
#include "component_registry.hpp"
#include "dll_component_registration.hpp"

using namespace Epoch::Engine::ECS;

#define BEGIN_COMPONENT(className, baseClass)                                                   \
    class className : public baseClass {                                                        \
public:                                                                                         \
        className(std::shared_ptr<Objects::Actor> parent, uint32_t local_id);

#define END_COMPONENT(className)                                                                \
    };                                                                                          \
    inline Components::Component* Create_##className() { return new className(nullptr, 0); }    

#define REGISTER_COMPONENT(className)                       \
namespace {                                                 \
    struct AutoRegister_##className {                       \
        AutoRegister_##className() {                        \
            Epoch::Engine::ECS::Components::AddComponentRegistrar( \
                [](Epoch::Engine::ECS::Components::ComponentRegistry& reg) { \
                    reg.RegisterComponentType(#className, Create_##className); \
                }                                           \
            );                                              \
        }                                                   \
    };                                                      \
    static AutoRegister_##className autoRegister_##className; \
}
