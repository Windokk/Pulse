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

        FIELD(Editable)
        glm::mat3 vectorTest = glm::mat3(1.0f);

        FIELD(Editable)
        float speed = 1.0f;
        
        FIELD(Editable)
        float mouseSensitivity = 0.1f;
        
        FIELD(ReadOnly)
        double lockedMouseX = 0; 
        
        FIELD(ReadOnly)
        double lockedMouseY = 0;
        
        FIELD(ReadOnly)
        bool firstClick = true;

        FIELD(ReadOnly)
        float pitch = 0.0f;
        
        FIELD(ReadOnly)
        float yaw = 0.0f;

    private:

        DECLARE_DESCRIPTOR(Character);
};

DECLARE_COMPONENT(Character)