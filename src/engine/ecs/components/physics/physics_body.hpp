#pragma once

#include <Jolt/Jolt.h>

#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/PlaneShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/Body.h>

#include "engine/physics/physics_system.hpp"

#include "engine/ecs/components/core/component.hpp"

#include "engine/rendering/utils.hpp"

#include "engine/rendering/debug/debug.hpp"

namespace Pulse::Engine::ECS::Components
{

    class PhysicsBody : public Component {
        private:
        JPH::BodyID mBodyID = JPH::BodyID();
        Physics::PhysicsShape shape;
        glm::vec3 scale;
        Rendering::DebugShape* debugShape = nullptr;
        EMotionType motionType = EMotionType::Static;

        public:
            PhysicsBody(std::shared_ptr<Objects::Actor> parent, uint32_t local_id);

            void CreateBody(Physics::PhysicsShape shape, glm::vec3 scale, EMotionType motionType);

            void Update(Physics::PhysicsShape shape, glm::vec3 scale, EMotionType motionType);

            void Tick();

            void Activate() override
            {
                Component::Activate();

                Physics::PhysicsSystem::GetBodyInterface().ActivateBody(mBodyID);
            }

            void DeActivate() override
            {
                Component::DeActivate();

                Physics::PhysicsSystem::GetBodyInterface().DeactivateBody(mBodyID);
            }

            void Destroy() override
            {
                RemoveBody();
            }

            void RemoveBody() {
                if (!mBodyID.IsInvalid()) {
                    Physics::PhysicsSystem::RemoveBody(mBodyID);
                    mBodyID = JPH::BodyID();
                }
            }

            void Deserialize(json componentData, json levelData) override;
            
            ordered_json Serialize() override;
            
            std::shared_ptr<Component> Clone() const override;
            
            Physics::PhysicsShape GetShapeType() { return shape; }
            Rendering::DebugShape* GetDebugShape()  { return debugShape; }
            JPH::BodyID GetBodyID() const { return mBodyID; }
            glm::vec3 GetScale() { return scale; }
            EMotionType GetMotionType() { return motionType; } 
    };
}