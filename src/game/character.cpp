#include "character.hpp"

#include "engine/ecs/objects/actors/actor.hpp"

#include "engine/core/engine.hpp"

#include "engine/core/platform/iplatform.hpp"

#include <thread>
#include <iostream>

using namespace Epoch::Engine;

Character::Character(Epoch::Engine::ECS::Objects::Actor* parent, uint32_t local_id)
    : Script(parent, local_id) {
    // Init
}

void Character::Deserialize(json componentData) {
    // Deserialize fields
}

json Character::Serialize() {
    return json();
}

void Character::Begin() {
}

void Character::Tick() {
    Core::Platform::IInput* input = Core::GetEngine().GetInputManager();

    if(input->IsKeyDown(Input::Key::W)){
        parent->transform->Translate(parent->transform->GetForward() * speed);
    }
    if(input->IsKeyDown(Input::Key::A)){
        parent->transform->Translate(glm::normalize(glm::cross(parent->transform->GetForward(), parent->transform->GetUp())) * -speed);
    }
    if(input->IsKeyDown(Input::Key::S)){
        parent->transform->Translate(parent->transform->GetForward() * -speed);
    }
    if(input->IsKeyDown(Input::Key::D)){
        parent->transform->Translate(glm::normalize(glm::cross(parent->transform->GetForward(), parent->transform->GetUp())) * speed);
    }

    if (input->IsMouseDown(Input::MouseButton::Left))
    {
        input->SetCursorVisibility(false);

        double mouseX, mouseY;
        input->GetCursorPos(&mouseX, &mouseY);

        if (firstClick)
        {
            lockedMouseX = mouseX;
            lockedMouseY = mouseY;
            firstClick = false;
        }

        double deltaX = mouseX - lockedMouseX;
        double deltaY = lockedMouseY - mouseY; // reversed Y

        pitch -= deltaY * mouseSensitivity;
        yaw   -= deltaX * mouseSensitivity;

        // Clamp pitch to avoid flipping
        pitch = glm::clamp(pitch, -89.0f, 89.0f);

        // Build quaternion from yaw * pitch
        glm::quat qPitch = glm::angleAxis(glm::radians(pitch), glm::vec3(1, 0, 0));
        glm::quat qYaw   = glm::angleAxis(glm::radians(yaw),   glm::vec3(0, 1, 0));
        glm::quat rotation = qYaw * qPitch;
        
        parent->transform->SetRotation(rotation);
 
        // Reset cursor back to locked position every frame
        input->SetCursorPos(lockedMouseX, lockedMouseY);
    }
    if(input->IsMouseUp(Input::MouseButton::Left))
    {
        firstClick = true;
        input->SetCursorVisibility(true);
    }
}


REGISTER_COMPONENT(Character);