#include "physics_body.hpp"

#include "engine/ecs/objects/actors/actor.hpp"
#include "engine/core/engine.hpp"

#include "physics_body.reflection.hpp"

#include <iostream>

namespace Pulse::Engine::ECS::Components{
    PhysicsBody::PhysicsBody(std::shared_ptr<Objects::Actor> parent, uint32_t local_id) : Component(parent, local_id)
    {
        
    }

    void PhysicsBody::Update(Physics::PhysicsShape shape, glm::vec3 scale, EMotionType motionType)
    {
        Core::GetEngine().GetPhysicsManager()->RemoveBody(mBodyID);
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

        mBodyID = Core::GetEngine().GetPhysicsManager()->CreateBody(settings, this);
    }

    void PhysicsBody::Tick(){

        if(!activated)
            return;

        JPH::RVec3 pos = Core::GetEngine().GetPhysicsManager()->GetBodyInterface().GetCenterOfMassPosition(mBodyID);
        JPH::Vec3 rot = Core::GetEngine().GetPhysicsManager()->GetBodyInterface().GetRotation(mBodyID).GetEulerAngles();
        
        this->parent->transform->SetPosition(glm::vec3(pos.GetX(), pos.GetY(), pos.GetZ()));
        this->parent->transform->SetRotation(glm::vec3(glm::degrees(rot.GetX()), glm::degrees(rot.GetY()), glm::degrees(rot.GetZ())));
    }

    void PhysicsBody::RemoveBody()
    {
        if (!mBodyID.IsInvalid()) {
            Core::GetEngine().GetPhysicsManager()->RemoveBody(mBodyID);
            mBodyID = JPH::BodyID();
        }
    }

    void PhysicsBody::Deserialize(json componentData, json levelData)
    {
        Physics::PhysicsShape shape;

        if(componentData["shape"] == "box"){
            shape = Physics::PhysicsShape::BOX;
        }
        else if(componentData["shape"] == "sphere"){
            shape = Physics::PhysicsShape::SPHERE;
        }
        else if(componentData["shape"] == "capsule"){
            shape = Physics::PhysicsShape::CAPSULE;
        }
        else if(componentData["shape"] == "cylinder"){
            shape = Physics::PhysicsShape::CYLINDER;
        }
        else{
            DEBUG_ERROR("Physics shape not recognized : " + (std::string)componentData["shape"]);
        }
        
        JPH::EMotionType motion;

        if(componentData["motion_type"] == "dynamic"){
            motion = JPH::EMotionType::Dynamic;
        }
        else if(componentData["motion_type"] == "kinematic"){
            motion = JPH::EMotionType::Kinematic;
        }
        else if(componentData["motion_type"] == "static"){
            motion = JPH::EMotionType::Static;
        }
        else{
            DEBUG_ERROR("Physics motion type not recognized : " + (std::string)componentData["shape"]);
        }

        CreateBody(shape, glm::vec3(componentData["size"]["x"], componentData["size"]["y"], componentData["size"]["z"]), motion);

        if(componentData.contains("active") && componentData["active"].get<bool>())
            Activate();
        else
            DeActivate();
    }

    ordered_json PhysicsBody::Serialize()
    {
        ordered_json comp;

        comp["type"] = "physics_body";

        comp["active"] = activated;

        switch(shape){
            case Physics::BOX:{
                comp["shape"] = "box";
                break;
            }
            case Physics::CAPSULE:{
                comp["shape"] = "capsule";
                break;
            }
            case Physics::CYLINDER:{
                comp["shape"] = "cylinder";
                break;
            }
            case Physics::SPHERE:{
                comp["shape"] = "sphere";
                break;
            }
        }

        comp["size"]["x"] = scale.x;
        comp["size"]["y"] = scale.y;
        comp["size"]["z"] = scale.z;

        switch(motionType){
            case JPH::EMotionType::Dynamic:{
                comp["motion_type"] = "dynamic";
                break;
            }
            case JPH::EMotionType::Static:{
                comp["motion_type"] = "static";
                break;
            }
            case JPH::EMotionType::Kinematic:{
                comp["motion_type"] = "kinematic";
                break;
            }
        }

        return comp;
    }

    std::shared_ptr<Component> PhysicsBody::Clone() const
    {
        auto cloned = std::make_shared<PhysicsBody>(*this);

        return cloned;
    }
}