#include "camera.hpp"

#include <iostream>

#include "engine/ecs/objects/actors/actor.hpp"

#include "engine/core/engine.hpp"

namespace Pulse::Engine::ECS::Components {
    
    Camera::Camera(std::shared_ptr<Objects::Actor> parent, uint32_t local_id) : Component(parent, local_id)
    {

    }

    void Camera::Init(int width, int height, float near, float far)
    {
        this->width = width;
        this->height = height;
        this->nearPlane = near;
        this->farPlane = far;
    }

    void Camera::Destroy()
    {
        Core::GetEngine().GetCameraManager()->RemoveCamera(parent->GetID());
    }

    void Camera::UpdateSize(int new_width, int new_height)
    {
        if(!activated)
            return;

        this->width = new_width;
        this->height = new_height;
    }

    void Camera::UpdateMatrix()
    {
        if(!activated)
            return;
            
        view = glm::mat4(1.0f);
        projection = glm::mat4(1.0f);

        std::shared_ptr<Transform> tr = parent->transform;

        view = glm::lookAt(tr->GetPosition(), tr->GetPosition() + tr->GetForward(), tr->GetUp());

        projection = glm::perspective(glm::radians(fov), float(width) / float(height), nearPlane, farPlane);
        cameraMatrix = projection * view;
    }

    Frustum createFrustumFromCamera(const Camera& cam, float aspect, float fovY,
                                                                float zNear, float zFar)
    {
        Frustum frustum;
        const float halfVSide = zFar * tanf(fovY * .5f);
        const float halfHSide = halfVSide * aspect;
        const glm::vec3 frontMultFar = zFar * cam.parent->transform->GetForward();

        frustum.nearFace = { cam.parent->transform->GetPosition() + zNear * cam.parent->transform->GetForward(), cam.parent->transform->GetForward()};
        frustum.farFace = { cam.parent->transform->GetPosition() + frontMultFar, -cam.parent->transform->GetForward() };
        frustum.rightFace = { cam.parent->transform->GetPosition(),
                                glm::cross(frontMultFar - cam.parent->transform->GetRight() * halfHSide, cam.parent->transform->GetUp()) };
        frustum.leftFace = { cam.parent->transform->GetPosition(),
                                glm::cross(cam.parent->transform->GetUp(),frontMultFar + cam.parent->transform->GetRight() * halfHSide) };
        frustum.topFace = { cam.parent->transform->GetPosition(),
                                glm::cross(cam.parent->transform->GetRight(), frontMultFar - cam.parent->transform->GetUp() * halfVSide) };
        frustum.bottomFace = { cam.parent->transform->GetPosition(),
                                glm::cross(frontMultFar + cam.parent->transform->GetUp() * halfVSide, cam.parent->transform->GetRight()) };

        return frustum;
    }

    bool isOnOrForwardPlane(glm::vec3 center, glm::vec3 extents, const Plane& plane)
	{
		// Compute the projection interval radius of b onto L(t) = b.c + t * p.n
		const float r = extents.x * std::abs(plane.normal.x) + extents.y * std::abs(plane.normal.y) +
			extents.z * std::abs(plane.normal.z);

		return -r <= plane.getSignedDistanceToPlane(center);
	}

    std::shared_ptr<Component> Camera::Clone() const
    {
        auto cloned = std::make_shared<Camera>(*this);

        return cloned;
    }

    bool Camera::IsInFrustum(glm::vec3 boundsMin, glm::vec3 boundsMax)
    {
        Frustum camFrustum = createFrustumFromCamera(*this, width/height, fov, nearPlane, farPlane);

        glm::vec3 center = (boundsMin + boundsMax) * 0.5f;
        glm::vec3 extents = boundsMax - boundsMin;

        return (isOnOrForwardPlane(center, extents, camFrustum.leftFace) &&
			isOnOrForwardPlane(center, extents, camFrustum.rightFace) &&
			isOnOrForwardPlane(center, extents, camFrustum.topFace) &&
			isOnOrForwardPlane(center, extents, camFrustum.bottomFace) &&
			isOnOrForwardPlane(center, extents, camFrustum.nearFace) &&
			isOnOrForwardPlane(center, extents, camFrustum.farFace));
    }

}