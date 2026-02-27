#pragma once

#include "engine/rendering/utils.hpp"

#include "engine/ecs/components/core/component.hpp"

namespace Pulse::Engine::ECS::Components {
    
    struct Plane
    {
        glm::vec3 normal = { 0.f, 1.f, 0.f }; // unit vector
        float     distance = 0.f;        // Distance with origin

        Plane() = default;

        Plane(const glm::vec3& p1, const glm::vec3& norm)
            : normal(glm::normalize(norm)),
            distance(glm::dot(normal, p1))
        {}

        float getSignedDistanceToPlane(const glm::vec3& point) const
        {
            return glm::dot(normal, point) - distance;
        }
    };

    struct Frustum
    {
        Plane topFace;
        Plane bottomFace;

        Plane rightFace;
        Plane leftFace;

        Plane farFace;
        Plane nearFace;
    };

    class CLASS() Camera : public Component
    {
        public :
            
            Camera(std::shared_ptr<Objects::Actor> parent, uint32_t local_id);

            void Init(int width, int height, float near, float far, float fov, bool ortho, float orthoSize);

            void Destroy() override;

            void UpdateSize(int new_width, int new_height);

            void UpdateMatrix();

            void SetOrthographic(bool ortho) { orthographic = ortho; }

            bool IsOrthographic() { return orthographic; }

            void SetFOV(float newFOV) { fov = newFOV; }

            float* GetFOV() { return &fov; }

            void SetOrthoSize(float orthoSize) { this->orthoSize = orthoSize; }

            float* GetOrthoSize() { return &orthoSize; }
 
            void OnFieldChanged(const FieldChangedEvent& event) override;

            void SetNearPlane(float newNear) { nearPlane = newNear; }
            void SetFarPlane(float newFar) { farPlane = newFar; }
            void SetNearFarPlanes(float newNear, float newFar) { nearPlane = newNear; farPlane = newFar; }

            void ToggleFrustumCulling() { frustumCulling = !frustumCulling; }

            void Deserialize(json componentData) override;
            
            ordered_json Serialize() override;
            
            std::shared_ptr<Component> Clone() const override;

            bool IsInFrustum(glm::vec3 boundsMin, glm::vec3 boundsMax);

            glm::vec3 GetWorldPointFromScreenPoint(glm::vec2 screenPoint);

            glm::mat4 GetMatrix()
            {
                return cameraMatrix;
            }

            glm::mat4 GetView()
            {
                return view;
            }

            glm::mat4 GetProjection()
            {
                return projection;
            }

            glm::vec2 GetSize()
            {
                return glm::vec2(width, height);
            }

            float farPlane = 0;
            float nearPlane = 0;

            bool orthographic = false;

            bool frustumCulling = true;

        private:

            float fov = 60.0f;

            float orthoSize = 10.0f;

            // Matrices
            glm::mat4 view;
            glm::mat4 projection;
            glm::mat4 cameraMatrix = glm::mat4(1.0f);

            // Store the width and height of the cam
            int width = 0;
            int height = 0;

            bool canInteract = true;
            
            DECLARE_DESCRIPTOR(Camera)
    };
}