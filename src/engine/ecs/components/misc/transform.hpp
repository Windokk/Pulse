#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "engine/ecs/components/core/component.hpp"

#include "engine/core/attributes.hpp"

namespace Pulse::Engine::ECS::Components
{
    class Transform : public Component{

        public:
        Transform(std::shared_ptr<Objects::Actor> parent, uint32_t local_id);

        void Deserialize(json componentData, json levelData) override;

        ordered_json Serialize() override;

        const glm::vec3& GetPosition() const { return position; };
        const glm::vec3 GetRotation() const { return glm::degrees(glm::eulerAngles(rotation)); };
        const glm::vec3& GetScale() const { return scale; };

        void SetPosition(glm::vec3 position);
        void SetRotation(glm::vec3 rotation);
        void SetRotation(glm::quat rotation);
        void SetScale(glm::vec3 scale);

        void Translate(glm::vec3 deltaPosition);
        void Rotate(glm::vec3 angle);
        void Scale(glm::vec3 deltaScale);

        void UpdateMeshReferencesInLevel();

        glm::vec3 GetForward() {
            return glm::normalize(rotation * glm::vec3(0, 0, 1));
        }

        glm::vec3 GetUp() {
            return glm::normalize(rotation * glm::vec3(0, 1, 0));
        }

        glm::vec3 GetRight() {
            return glm::normalize(rotation * glm::vec3(1, 0, 0));
        }

        glm::mat4 GetTransformMatrix();

        bool SetFromTransformMatrix(const glm::mat4 &m);

        void Destroy() override{
            //TODO ?
        }

        std::shared_ptr<Component> Clone() const override;

        private:

            ATTRIBUTE(Editable, write=WritePositionToTransform)
            glm::vec3 position = glm::vec3(0);

            ATTRIBUTE(Editable, read=ReadRotationFromTransform, write=WriteRotationToTransform, type=glm::vec3) 
            glm::quat rotation = glm::quat(glm::vec3(0, 0, 0));

            ATTRIBUTE(Editable, write=WriteScaleToTransform) 
            glm::vec3 scale = glm::vec3(1);

            DECLARE_DESCRIPTOR(Transform)
    };
}

inline void WritePositionToTransform(void* object, const void* value) {
    Pulse::Engine::ECS::Components::Transform* comp = static_cast<Pulse::Engine::ECS::Components::Transform*>(object);
    const glm::vec3* pos = static_cast<const glm::vec3*>(value);
    comp->SetPosition(*pos);
}

inline void ReadRotationFromTransform(void* object, void* outValue) {
    Pulse::Engine::ECS::Components::Transform* comp = static_cast<Pulse::Engine::ECS::Components::Transform*>(object);
    *static_cast<glm::vec3*>(outValue) = comp->GetRotation();
}

inline void WriteRotationToTransform(void* object, const void* value) {
    Pulse::Engine::ECS::Components::Transform* comp = static_cast<Pulse::Engine::ECS::Components::Transform*>(object);
    const glm::vec3* rot = static_cast<const glm::vec3*>(value);
    comp->SetRotation(*rot);
}

inline void WriteScaleToTransform(void* object, const void* value) {
    Pulse::Engine::ECS::Components::Transform* comp = static_cast<Pulse::Engine::ECS::Components::Transform*>(object);
    const glm::vec3* sca = static_cast<const glm::vec3*>(value);
    comp->SetScale(*sca);
}