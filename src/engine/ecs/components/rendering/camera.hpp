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

    class Camera : public Component
    {
        public :
            
            Camera(std::shared_ptr<Objects::Actor> parent, uint32_t local_id);

            void Init(int width, int height, float near, float far);

            void Destroy() override;

            void UpdateSize(int new_width, int new_height);

            void UpdateMatrix();

            void SetFOV(float newFOV) { fov = newFOV; }

            void SetNearPlane(float newNear) { nearPlane = newNear; }
            void SetFarPlane(float newFar) { farPlane = newFar; }
            void SetNearFarPlanes(float newNear, float newFar) { nearPlane = newNear; farPlane = newFar; }

            void ToggleFrustumCulling() { frustumCulling = !frustumCulling; }
            
            std::shared_ptr<Component> Clone() const override;

            bool IsInFrustum(glm::vec3 boundsMin, glm::vec3 boundsMax);

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

            float fov = 60.0f;

            bool frustumCulling = true;

        private:

            // Matrices
            glm::mat4 view;
            glm::mat4 projection;
            glm::mat4 cameraMatrix = glm::mat4(1.0f);

            // Mouse inputs
            float mouseSensitivity = 0.1f;
            bool firstClick = true;
            double originalmouseX = 0.0, originalmouseY = 0.0;
            double lockedMouseY = 0.0;
            double lockedMouseX = 0.0;

            // Store the width and height of the window
            int width = 0;
            int height = 0;

            bool canInteract = true;
    };
}