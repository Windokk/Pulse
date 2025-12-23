#include "engine/ecs/components/misc/script.hpp"
#include "engine/ecs/components/core/registry/component_macros.hpp"

using namespace nlohmann;

BEGIN_COMPONENT(Character, Pulse::Engine::ECS::Components::Script)

    void Deserialize(json componentData, json levelData) override;
    ordered_json Serialize() override;

    void Begin() override;
    void Tick() override;

    float speed = 1.0f;
    double lockedMouseX, lockedMouseY = 0;
    bool firstClick = true;

    float pitch = 0.0f;
    float yaw = 0.0f;

    float mouseSensitivity = 0.1f;

END_COMPONENT(Character)