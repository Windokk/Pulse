#include "physics_body.hpp"

#include "engine/ecs/objects/actors/actor.hpp"
#include "engine/core/engine.hpp"

#include "physics_body.reflection.hpp"

#include <iostream>

namespace Pulse::Engine::ECS::Components{
    PhysicsBody::PhysicsBody(std::shared_ptr<Objects::Actor> parent, uint32_t local_id) : Component(parent, local_id)
    {
        
    }

    void PhysicsBody::Update(Physics::PhysicsShape shape, std::shared_ptr<ShapeParams> params, EMotionType motionType)
    {
        Core::GetEngine().GetPhysicsManager()->RemoveBody(mBodyID);
        CreateBody(shape, params, motionType);
    }

    void PhysicsBody::CreateBody(Physics::PhysicsShape shape, std::shared_ptr<ShapeParams> params, EMotionType motionType)
    {
        JPH::ShapeRefC shapeRef;

        this->params = params;
        this->shape = shape;
        this->motionType = motionType;

        switch (shape) {
            case Physics::PhysicsShape::SPHERE: {

                auto sphereParams = std::dynamic_pointer_cast<SphereParams>(params);
                if (!sphereParams) return;

                JPH::SphereShapeSettings sphereSettings(sphereParams->radius);
                auto shapeResult = sphereSettings.Create();
                if (shapeResult.HasError()) {
                    std::cerr << "Failed to create sphere shape: " << shapeResult.GetError() << "\n";
                    return;
                }
                shapeRef = shapeResult.Get();
                debugShape = new Rendering::DebugSphere(0.5f, COL_RGBA(0, 1, 1, 1));
                break;
            }
            case Physics::PhysicsShape::BOX:{

                auto boxParams = std::dynamic_pointer_cast<BoxParams>(params);
                if (!boxParams) return;

                JPH::BoxShapeSettings boxSettings(JPH::Vec3(boxParams->halfExtent.x, boxParams->halfExtent.y, boxParams->halfExtent.z));
                auto shapeResult = boxSettings.Create();
                if (shapeResult.HasError()) { 
                    std::cerr << "Failed to create cube shape: " << shapeResult.GetError() << "\n";
                    return;
                }

                shapeRef = shapeResult.Get();
                debugShape = new Rendering::DebugBox(glm::vec3(0.5f), COL_RGBA(0, 1, 1, 1));
                break;
            }
            case Physics::PhysicsShape::CAPSULE: {

                auto capsuleParams = std::dynamic_pointer_cast<CapsuleParams>(params);
                if (!capsuleParams) return;

                JPH::CapsuleShapeSettings capsuleSettings(capsuleParams->halfHeight, capsuleParams->radius);
                auto shapeResult = capsuleSettings.Create();
                if (shapeResult.HasError()) { 
                    std::cerr << "Failed to create capsule shape: " << shapeResult.GetError() << "\n";
                    return;
                }

                shapeRef = shapeResult.Get();
                debugShape = new Rendering::DebugCapsule(0.5f, 0.5f, COL_RGBA(0, 1, 1, 1));
                break;
            }
            case Physics::PhysicsShape::CYLINDER:{
                
                auto cylinderParams = std::dynamic_pointer_cast<CylinderParams>(params);
                if (!cylinderParams) return;

                JPH::CylinderShapeSettings cylinderSettings(cylinderParams->halfHeight, cylinderParams->radius);
                auto shapeResult = cylinderSettings.Create();
                if (shapeResult.HasError()) { 
                    std::cerr << "Failed to create cylinder shape: " << shapeResult.GetError() << "\n";
                    return;
                }

                shapeRef = shapeResult.Get();
                debugShape = new Rendering::DebugCylinder(0.5f, 0.5f, COL_RGBA(0, 1, 1, 1));
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
            motionType == EMotionType::Static ? Physics::Layers::NON_MOVING : Physics::Layers::MOVING
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

    void PhysicsBody::Activate()
    {
        Component::Activate();

        Core::GetEngine().GetPhysicsManager()->GetBodyInterface().ActivateBody(mBodyID);
    }

    void PhysicsBody::DeActivate()
    {
        Component::DeActivate();

        Core::GetEngine().GetPhysicsManager()->GetBodyInterface().DeactivateBody(mBodyID);
    }

    void PhysicsBody::RemoveBody()
    {
        if (!mBodyID.IsInvalid()) {
            Core::GetEngine().GetPhysicsManager()->RemoveBody(mBodyID);
            mBodyID = JPH::BodyID();
        }
    }

    void PhysicsBody::Deserialize(json componentData)
    {
        Physics::PhysicsShape shape;

        if(componentData.contains("shape")){
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
                return;
            }
        }
        else{
            DEBUG_ERROR("Physics shape type not specified for physics body on actor : " + (std::string)parent->GetName());
            return;
        }
        
        JPH::EMotionType motion;
        if(componentData.contains("motion_type")){
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
                DEBUG_ERROR("Physics motion type not recognized : " + (std::string)componentData["motion_type"]);
                return;
            }
        }
        else{
            DEBUG_ERROR("Physics motion type not specified for physics body on actor : " + (std::string)parent->GetName());
            return;
        }
        

        std::shared_ptr<ShapeParams> params;
        if(componentData.contains("params")){
            switch(shape){
                case(Physics::PhysicsShape::BOX):{
                    if(componentData["params"].contains("x") && componentData["params"]["x"].is_number_float() && componentData["params"].contains("y") && componentData["params"]["y"].is_number_float() && componentData["params"].contains("z") && componentData["params"]["z"].is_number_float())
                    {
                        params = std::make_shared<BoxParams>(glm::vec3(componentData["params"]["x"].get<float>(), componentData["params"]["y"].get<float>(), componentData["params"]["z"].get<float>()));
                    }
                    else{
                        DEBUG_ERROR("Physics shape params are not valid for physics body on actor : " + (std::string)parent->GetName());
                        return;
                    }
                    break;
                }
                case(Physics::PhysicsShape::SPHERE):{
                    if(componentData["params"].contains("radius") && componentData["params"]["radius"].is_number_float())
                    {
                        params = std::make_shared<SphereParams>(componentData["params"]["radius"].get<float>());
                    }
                    else{
                        DEBUG_ERROR("Physics shape params are not valid for physics body on actor : " + (std::string)parent->GetName());
                        return;
                    }
                    break;
                }
                case(Physics::PhysicsShape::CAPSULE):{
                    if(componentData["params"].contains("radius") && componentData["params"]["radius"].is_number_float() && componentData["params"].contains("halfHeight") && componentData["params"]["halfHeight"].is_number_float())
                    {
                        params = std::make_shared<CapsuleParams>(componentData["params"]["radius"].get<float>(), componentData["params"]["halfHeight"].get<float>());
                    }
                    else{
                        DEBUG_ERROR("Physics shape params are not valid for physics body on actor : " + (std::string)parent->GetName());
                        return;
                    }
                    break;
                }
                case(Physics::PhysicsShape::CYLINDER):{
                    if(componentData["params"].contains("radius") && componentData["params"]["radius"].is_number_float() && componentData["params"].contains("halfHeight") && componentData["params"]["halfHeight"].is_number_float())
                    {
                        params = std::make_shared<CylinderParams>(componentData["params"]["radius"].get<float>(), componentData["params"]["halfHeight"].get<float>());
                    }
                    else{
                        DEBUG_ERROR("Physics shape params are not valid for physics body on actor : " + (std::string)parent->GetName());
                        return;
                    }
                    break;
                }
            }
        }
        else{
            DEBUG_ERROR("Physics shape parameters not specified for physics body on actor : " + (std::string)parent->GetName());
            return;
        }

        CreateBody(shape, params, motion);

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
                auto boxParams = std::dynamic_pointer_cast<BoxParams>(params);
                comp["shape"] = "box";
                comp["params"]["x"] = boxParams->halfExtent.x;
                comp["params"]["y"] = boxParams->halfExtent.y;
                comp["params"]["z"] = boxParams->halfExtent.z;
                break;
            }
            case Physics::CAPSULE:{
                auto capsuleParams = std::dynamic_pointer_cast<CapsuleParams>(params);
                comp["shape"] = "capsule";
                comp["params"]["radius"] = capsuleParams->radius;
                comp["params"]["halfHeight"] = capsuleParams->halfHeight;
                break;
            }
            case Physics::CYLINDER:{
                auto cylinderParams = std::dynamic_pointer_cast<CylinderParams>(params);
                comp["shape"] = "cylinder";
                comp["params"]["radius"] = cylinderParams->radius;
                comp["params"]["halfHeight"] = cylinderParams->halfHeight;
                break;
            }
            case Physics::SPHERE:{
                auto sphereParams = std::dynamic_pointer_cast<SphereParams>(params);
                comp["shape"] = "sphere";
                comp["params"]["radius"] = sphereParams->radius;
                break;
            }
        }

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