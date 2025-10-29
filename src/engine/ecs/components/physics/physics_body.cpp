#include "physics_body.hpp"

#include "engine/ecs/objects/actors/actor.hpp"

#include <iostream>

namespace Pulse::Engine::ECS::Components{
    PhysicsBody::PhysicsBody(std::shared_ptr<Objects::Actor> parent, uint32_t local_id) : Component(parent, local_id)
    {
        
    }

    void PhysicsBody::Update(Physics::PhysicsShape shape, glm::vec3 scale, EMotionType motionType)
    {
        Physics::PhysicsSystem::RemoveBody(mBodyID);
        CreateBody(shape, scale, motionType);
    }

    void PhysicsBody::CreateBody(Physics::PhysicsShape shape, glm::vec3 scale, EMotionType motionType)
    {
        JPH::ShapeRefC shapeRef;

        this->scale = scale;
        this->shape = shape;
        this->motionType = motionType;

        switch (shape) {
            case Physics::PhysicsShape::SPHERE: {
                JPH::SphereShapeSettings sphereSettings(scale.x == 0 ? (scale.y == 0 ? scale.z : scale.y) : scale.x);
                auto shapeResult = sphereSettings.Create();
                if (shapeResult.HasError()) {
                    std::cerr << "Failed to create sphere shape: " << shapeResult.GetError() << "\n";
                    return;
                }

                shapeRef = shapeResult.Get();
                debugShape = new Rendering::DebugSphere((scale.x == 0 ? (scale.y == 0 ? scale.z : scale.y) : scale.x), COL_RGBA(0, 1, 1, 1));
                break;
            }
            case Physics::PhysicsShape::BOX:{
                JPH::BoxShapeSettings boxSettings(JPH::Vec3(scale.x, scale.y, scale.z));
                auto shapeResult = boxSettings.Create();
                if (shapeResult.HasError()) { 
                    std::cerr << "Failed to create cube shape: " << shapeResult.GetError() << "\n";
                    return;
                }

                shapeRef = shapeResult.Get();
                debugShape = new Rendering::DebugBox(glm::vec3(scale.x, scale.y, scale.z), COL_RGBA(0, 1, 1, 1));
                break;
            }
            case Physics::PhysicsShape::CAPSULE: {
                JPH::CapsuleShapeSettings capsuleSettings(scale.y, scale.x == 0 ? scale.z : scale.x);
                auto shapeResult = capsuleSettings.Create();
                if (shapeResult.HasError()) { 
                    std::cerr << "Failed to create capsule shape: " << shapeResult.GetError() << "\n";
                    return;
                }

                shapeRef = shapeResult.Get();
                debugShape = new Rendering::DebugCapsule(scale.y, scale.x == 0 ? scale.z : scale.x, COL_RGBA(0, 1, 1, 1));
                break;
            }
            case Physics::PhysicsShape::CYLINDER:{
                JPH::CylinderShapeSettings cylinderSettings(scale.y, scale.x == 0 ? scale.z : scale.x);
                auto shapeResult = cylinderSettings.Create();
                if (shapeResult.HasError()) { 
                    std::cerr << "Failed to create cylinder shape: " << shapeResult.GetError() << "\n";
                    return;
                }

                shapeRef = shapeResult.Get();
                debugShape = new Rendering::DebugCylinder(scale.y, scale.x == 0 ? scale.z : scale.x, COL_RGBA(0, 1, 1, 1));
                break;
            }

            default:
                DEBUG_ERROR("Unsupported physics shape type !");
                return;
        }

        glm::vec3 pos = parent->transform->GetPosition();
        glm::quat rot = parent->transform->GetRotation();
        
        // Body settings
        JPH::BodyCreationSettings settings(
            shapeRef,
            JPH::RVec3(pos.x, pos.y, pos.z),        // position
            JPH::Quat(rot.x, rot.y, rot.z, rot.w),  // rotation
            motionType,                             // motion type
            1
        );

        mBodyID = Physics::PhysicsSystem::CreateBody(settings, this);
    }

    void PhysicsBody::Tick(){

        if(!activated)
            return;

        JPH::RVec3 pos = Physics::PhysicsSystem::GetBodyInterface().GetCenterOfMassPosition(mBodyID);
        JPH::Vec3 rot = Physics::PhysicsSystem::GetBodyInterface().GetRotation(mBodyID).GetEulerAngles();
        
        this->parent->transform->SetPosition(glm::vec3(pos.GetX(), pos.GetY(), pos.GetZ()));
        this->parent->transform->SetRotation(glm::vec3(glm::degrees(rot.GetX()), glm::degrees(rot.GetY()), glm::degrees(rot.GetZ())));
    }

    std::shared_ptr<Component> PhysicsBody::Clone() const
    {
        auto cloned = std::make_shared<PhysicsBody>(*this);

        return cloned;
    }
}