#include "camera.hpp"

#include <iostream>

#include "engine/ecs/objects/actors/actor.hpp"

#include "camera.reflection.hpp"

#include "engine/core/engine.hpp"

namespace Pulse::Engine::ECS::Components {
    
    Camera::Camera(std::shared_ptr<Objects::Actor> parent, uint32_t local_id) : Component(parent, local_id)
    {

    }

    void Camera::Init(int width, int height, float near, float far, float fov, bool ortho, float orthoSize)
    {
        this->width = width;
        this->height = height;
        this->nearPlane = near;
        this->farPlane = far;
        this->fov = fov;
        this->orthographic = ortho;
        this->orthoSize = orthoSize;
    }

    void Camera::Destroy()
    {
        Core::GetEngine().GetCameraManager()->RemoveCamera(parent->GetID());
    }

    void Camera::OnFieldChanged(const FieldChangedEvent &event)
    {
        if(event.field->name == "width"){

        }
        else if(event.field->name == "height"){

        }
        else if(event.field->name == "near"){
            
        }
        else if(event.field->name == "far"){

        }
        else if(event.field->name == "fov"){

        }
        else if(event.field->name == "frustumCulling"){

        }
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
        if (!activated || parent == nullptr || parent->transform == nullptr)
            return;

        // Reset matrices
        view = glm::mat4(1.0f);
        projection = glm::mat4(1.0f);

        std::shared_ptr<Transform> tr = parent->transform;

        glm::vec3 position = tr->GetPosition();
        glm::vec3 forward  = tr->GetForward();
        glm::vec3 up       = tr->GetUp();

        // View matrix
        view = glm::lookAt(position, position + forward, up);

        // Avoid division by zero
        float aspect = (height != 0) ? float(width) / float(height) : 1.0f;

        if (orthographic)
        {
            // Orthographic projection
            float right = orthoSize * aspect;
            float left  = -right;
            float top   = orthoSize;
            float bottom= -top;

            projection = glm::ortho(left, right, bottom, top, nearPlane, farPlane);
        }
        else
        {
            // Perspective projection
            projection = glm::perspective(
                glm::radians(fov),
                aspect,
                nearPlane,
                farPlane
            );
        }

        // Final camera matrix
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
		const float r = extents.x * std::abs(plane.normal.x) +
            extents.y * std::abs(plane.normal.y) + extents.z * std::abs(plane.normal.z);

		return -r <= plane.getSignedDistanceToPlane(center);
	}

    bool Camera::IsInFrustum(glm::vec3 boundsMin, glm::vec3 boundsMax)
    {
        float aspect = float(width) / float(height);
        Frustum camFrustum = createFrustumFromCamera(*this, aspect, fov, nearPlane, farPlane);

        glm::vec3 center = (boundsMin + boundsMax) * 0.5f;
        glm::vec3 extents = boundsMax - boundsMin;

        return (isOnOrForwardPlane(center, extents, camFrustum.leftFace) &&
			isOnOrForwardPlane(center, extents, camFrustum.rightFace) &&
			isOnOrForwardPlane(center, extents, camFrustum.topFace) &&
			isOnOrForwardPlane(center, extents, camFrustum.bottomFace) &&
			isOnOrForwardPlane(center, extents, camFrustum.nearFace) &&
			isOnOrForwardPlane(center, extents, camFrustum.farFace));
    }

    glm::vec3 Camera::GetWorldPointFromScreenPoint(glm::vec2 screenPoint)
    {
        float x = (2.0f * screenPoint.x) / width - 1.0f;
        float y = 1.0f - (2.0f * screenPoint.y) / height;
        float z = 1.0f;
        glm::vec3 ray_nds = glm::vec3(x, y, z);
        glm::vec4 ray_clip = glm::vec4(ray_nds.x, ray_nds.y, -1.0, 1.0);

        glm::vec4 ray_eye = glm::inverse(projection) * ray_clip;
        ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0, 0.0);

        glm::vec3 ray_wor = glm::inverse(view) * ray_eye;
        return glm::normalize(ray_wor);
    }

    void Camera::Deserialize(json componentData)
    {
        int width = Core::GetEngine().GetWindow()->GetFramebufferWidth();
        int height = Core::GetEngine().GetWindow()->GetFramebufferHeight();

        Init(width, height, componentData["near"], componentData["far"], componentData["fov"], componentData["orthographic"], componentData["orthoSize"]);
        
        if(componentData.contains("active") && componentData["active"].get<bool>())
            Activate();
        else
            DeActivate();
    }

    ordered_json Camera::Serialize()
    {
        ordered_json comp;

        comp["type"] = "camera";

        comp["orthographic"] = orthographic;

        comp["active"] = activated;

        comp["near"] = nearPlane;

        comp["far"] = farPlane;

        comp["fov"] = fov;

        comp["orthoSize"] = orthoSize;

        return comp;
    }

    std::shared_ptr<Component> Camera::Clone() const
    {
        auto cloned = std::make_shared<Camera>(*this);

        return cloned;
    }
}