#pragma once

#include <nlohmann/json.hpp>

#include "engine/levels/level.hpp"
#include "engine/ecs/objects/actors/actor.hpp"

#include <memory>
#include <typeinfo>

#include "engine/core/reflection_fields.hpp"

namespace Pulse::Editor::GUI{

    class EditorMainWindow;

    class PropertiesPanel
    {
        public:
            void Draw(std::shared_ptr<Engine::ECS::Objects::Actor> actor);

        private:
            void DrawActorInfo(std::shared_ptr<Engine::ECS::Objects::Actor> actor);
            void DrawComponent(std::shared_ptr<Engine::ECS::Components::Component> comp);
            void DrawField(const FieldInfo* field, void* value,
                        std::shared_ptr<Engine::ECS::Components::Component> comp,
                        const Container* container = nullptr);
    };
}