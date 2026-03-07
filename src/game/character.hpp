#pragma once

#include "engine/ecs/components/misc/script.hpp"
#include "engine/ecs/components/core/registry/component_macros.hpp"

using namespace nlohmann;

class CLASS() Character : public Pulse::Engine::ECS::Components::Script{

    public:
        Character(std::shared_ptr<Objects::Actor> parent, uint32_t local_id) : Script(parent, local_id){};

        void Deserialize(json componentData) override;
        ordered_json Serialize() override;

        void OnPlay() override;
        void OnTick() override;
        void OnStop() override;

    private:
        float speed = 1.0f;
        double lockedMouseX, lockedMouseY = 0;
        bool firstClick = true;

        float pitch = 0.0f;
        float yaw = 0.0f;

        float mouseSensitivity = 0.1f;

        DECLARE_DESCRIPTOR(Character);
};

DECLARE_COMPONENT(Character)