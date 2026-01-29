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

#include "engine/physics/physics_manager.hpp"

#include "engine/ecs/components/core/component.hpp"

#include "engine/rendering/utils.hpp"

#include "engine/rendering/debug/debug.hpp"

namespace Pulse::Engine::ECS::Components
{

    struct ShapeParams {
        virtual ~ShapeParams() = default; // <- very important!
    };

    struct STRUCT() SphereParams : public ShapeParams {
        FIELD(Editable) float radius;
        SphereParams(float r) : radius(r) {}
    };

    
    struct STRUCT() CapsuleParams : public ShapeParams {
        FIELD(Editable) float radius;
        FIELD(Editable) float halfHeight;
        CapsuleParams(float r, float h) : radius(r), halfHeight(h) {}
    };

    struct STRUCT() BoxParams : public ShapeParams{
        FIELD(Editable) glm::vec3 halfExtent;
        BoxParams(glm::vec3 e) : halfExtent(e) {}
    };

    struct STRUCT() CylinderParams : public ShapeParams{
        FIELD(Editable) float radius;
        FIELD(Editable) float halfHeight;
        CylinderParams(float r, float h) : radius(r), halfHeight(h) {}
    };

    class CLASS() PhysicsBody : public Component {
        
        public:
            PhysicsBody(std::shared_ptr<Objects::Actor> parent, uint32_t local_id);

            void CreateBody(Physics::PhysicsShape shape, std::shared_ptr<ShapeParams> params, EMotionType motionType);

            void ApplyTransformToPhysics(float dt);

            void SyncTransformFromPhysics();

            void Update(const Physics::PhysicsShape& shape, const std::shared_ptr<ShapeParams>& params, EMotionType motionType, bool forceRecreation = false);

            void Tick(float dt);

            void SetPosition(glm::vec3 newPos);

            void SetRotation(glm::vec3 newRot);

            void SetRotation(glm::quat newRot);

            void Activate() override;

            void DeActivate() override;

            void Destroy() override
            {
                RemoveBody();
            }

            void RemoveBody();

            void Deserialize(json componentData) override;
            
            ordered_json Serialize() override;
            
            std::shared_ptr<Component> Clone() const override;
            
            Physics::PhysicsShape GetShapeType() { return shape; }
            std::shared_ptr<ShapeParams> GetShapeParams() { return params; }
            Rendering::DebugShape* GetDebugShape() { return debugShape; }
            JPH::BodyID GetBodyID() const { return mBodyID; }
            EMotionType GetMotionType() { return motionType; } 

            DECLARE_DESCRIPTOR(PhysicsBody)

        private:

            JPH::ShapeRefC CreateJoltShape(Physics::PhysicsShape shape, const std::shared_ptr<ShapeParams> &params);

            JPH::BodyID mBodyID = JPH::BodyID();
            FIELD(Editable) Physics::PhysicsShape shape;
            std::shared_ptr<ShapeParams> params;
            Rendering::DebugShape* debugShape = nullptr;
            FIELD(Editable) EMotionType motionType = EMotionType::Static;
            JPH::ShapeRefC mShape;

    };
}