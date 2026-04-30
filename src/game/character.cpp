#include "character.hpp"

#include "engine/ecs/objects/actors/actor.hpp"

#include "engine/core/engine.hpp"

#include "engine/core/platform/iplatform.hpp"

#include <thread>
#include <iostream>

#include "engine/ecs/components/misc/script.reflection.hpp"

using namespace Pulse::Engine;

void Character::Deserialize(const json componentData) {
    // Deserialize fields
    if(componentData.contains("active") && componentData["active"].is_boolean() && componentData["active"]){
        Activate();
    }
    else{
        DeActivate();
    }
}

ordered_json Character::Serialize() {
    
    ordered_json comp;

    comp["type"] = "Character";

    comp["active"] = activated;

    return comp;
}

void Character::OnPlay() {
}

void Character::OnTick() {
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

        pitch += deltaY * mouseSensitivity;
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

void Character::OnStop()
{
    Core::GetEngine().GetInputManager()->SetCursorVisibility(true);
}

REGISTER_COMPONENT(Character);