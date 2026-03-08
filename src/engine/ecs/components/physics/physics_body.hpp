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

#include "engine/core/reflection_fields.hpp"

namespace Pulse::Engine::ECS::Components
{

    struct STRUCT() SphereParams{
        FIELD(Editable) float radius = 0.5f;
        SphereParams(float r) : radius(r) {}
        SphereParams() = default;
        ~SphereParams() = default;

        inline bool operator==(const SphereParams& other) const {
            const float epsilon = 1e-6f;
            return std::abs(radius - other.radius) < epsilon;
        }
    };

    
    struct STRUCT() CapsuleParams{
        FIELD(Editable) float radius = 0.5f;
        FIELD(Editable) float halfHeight = 0.5f;
        CapsuleParams(float r, float h) : radius(r), halfHeight(h) {}
        CapsuleParams() = default;
        ~CapsuleParams() = default;

        inline bool operator==(const CapsuleParams& other) const {
            const float epsilon = 1e-6f;
            return std::abs(radius - other.radius) < epsilon &&
                std::abs(halfHeight - other.halfHeight) < epsilon;
        }
    };

    struct STRUCT() BoxParams{
        FIELD(Editable) glm::vec3 halfExtent = glm::vec3(0.5f);
        BoxParams(glm::vec3 e) : halfExtent(e) {}
        BoxParams() = default;
        ~BoxParams() = default;

        inline bool operator==(const BoxParams& other) const {
            return halfExtent == other.halfExtent;
        }
    };

    struct STRUCT() CylinderParams{
        FIELD(Editable) float radius = 0.5f;
        FIELD(Editable) float halfHeight = 0.5f;
        CylinderParams(float r, float h) : radius(r), halfHeight(h) {}
        CylinderParams() = default;
        ~CylinderParams() = default;

        inline bool operator==(const CylinderParams& other) const {
            const float epsilon = 1e-6f;
            return std::abs(radius - other.radius) < epsilon &&
                std::abs(halfHeight - other.halfHeight) < epsilon;
        }
    };

    class CLASS() PhysicsBody : public Component {
        
        public:
            PhysicsBody(std::shared_ptr<Objects::Actor> parent, uint32_t local_id);

            void CreateBody(Physics::PhysicsShape shape, const InstancedStruct& params, EMotionType motionType);

            void ApplyTransformToPhysics(float dt);

            void SyncTransformFromPhysics();

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
            
            Physics::PhysicsShape GetShapeType() { return shapeType; }

            template<typename T>
            T& GetShapeParams();

            Rendering::DebugShape* GetDebugShape() { return debugShape; }
            JPH::BodyID GetBodyID() const { return mBodyID; }
            EMotionType GetMotionType() { return motionType; } 

            void ForceShapeUpdate(const Physics::PhysicsShape& shape, const InstancedStruct& params, EMotionType motionType);

            void OnFieldChanged(const FieldChangedEvent& event) override;

            FIELD(Editable) 
            InstancedStruct params;

            FIELD(Editable) 
            Physics::PhysicsShape shapeType;
            
            FIELD(Editable)
            EMotionType motionType = EMotionType::Static;

        private:
        
            bool shouldUpdateShape = false;

            void Update(const Physics::PhysicsShape& shape, const InstancedStruct& params, EMotionType motionType, bool forceRecreation = false);

            JPH::ShapeRefC CreateJoltShape(Physics::PhysicsShape shape, const InstancedStruct& params);

            JPH::BodyID mBodyID = JPH::BodyID();

            Rendering::DebugShape* debugShape = nullptr;

            JPH::ShapeRefC mShape;
            
            DECLARE_DESCRIPTOR(PhysicsBody)
    };

    template<typename T>
    T& PhysicsBody::GetShapeParams()
    {
        if(!std::is_base_of<BoxParams, T>::value && !std::is_base_of<SphereParams, T>::value &&
             !std::is_base_of<CylinderParams, T>::value && !std::is_base_of<CapsuleParams, T>::value){
            return nullptr;
        }
        return *static_cast<T*>(params.data);
    }
}

using namespace Pulse::Engine;

inline void ReadPhysicsShape(void* object, void* outValue) {
    ECS::Components::PhysicsBody* comp = static_cast<ECS::Components::PhysicsBody*>(object);
    *static_cast<int*>(outValue) = static_cast<int>(comp->GetShapeType());
}

inline void WritePhysicsShape(void* object, const void* value);

inline void WriteShapeParams(void* object, const void* value);