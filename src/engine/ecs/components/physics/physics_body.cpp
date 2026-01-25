#include "physics_body.hpp"

#include "engine/ecs/objects/actors/actor.hpp"
#include "engine/core/engine.hpp"

#include "physics_body.reflection.hpp"

#include <iostream>

#include <thread>

namespace Pulse::Engine::ECS::Components{
    PhysicsBody::PhysicsBody(std::shared_ptr<Objects::Actor> parent, uint32_t local_id) : Component(parent, local_id)
    {
        
    }

    void PhysicsBody::Update(const Physics::PhysicsShape& newShape, const std::shared_ptr<ShapeParams>& newParams, EMotionType newMotionType, bool forceRecreation)
    {
        auto physics = Core::GetEngine().GetPhysicsManager();
        if (!physics || mBodyID.IsInvalid())
            return;

        auto& bi = physics->GetBodyInterface();

        // --- Shape update ---
        if (newShape != shape || newParams != params || forceRecreation)
        {
            JPH::ShapeRefC newShapeRef = CreateJoltShape(newShape, newParams);
            if (!newShapeRef)
                return;

            bi.SetShape(
                mBodyID,
                newShapeRef,
                /*updateMassProperties=*/newMotionType == EMotionType::Dynamic,
                JPH::EActivation::Activate
            );

            mShape = newShapeRef;
            shape = newShape;
            params = newParams;
        }

        // --- Motion type update ---
        if (newMotionType != motionType)
        {
            bi.SetMotionType(
                mBodyID,
                newMotionType,
                JPH::EActivation::Activate
            );

            motionType = newMotionType;
        }
    }

    JPH::ShapeRefC PhysicsBody::CreateJoltShape(Physics::PhysicsShape shape, const std::shared_ptr<ShapeParams>& params)
    {
        glm::vec3 scale = parent->transform->GetScale();

        if(debugShape) {
            delete debugShape;
            debugShape = nullptr;
        }

        switch (shape)
        {
            case Physics::PhysicsShape::SPHERE:
            {
                auto p = std::dynamic_pointer_cast<SphereParams>(params);
                if (!p) return nullptr;

                float size = std::max({ scale.x, scale.y, scale.z });

                JPH::SphereShapeSettings s(p->radius * size);

                debugShape = new Rendering::DebugSphere(0.5f, COL_RGBA(0, 1, 1, 1));

                auto r = s.Create();
                return r.HasError() ? nullptr : r.Get();
            }

            case Physics::PhysicsShape::BOX:
            {
                auto p = std::dynamic_pointer_cast<BoxParams>(params);
                if (!p) return nullptr;

                JPH::BoxShapeSettings s(
                    JPH::Vec3(p->halfExtent.x * scale.x, p->halfExtent.y * scale.y, p->halfExtent.z * scale.z)
                );

                debugShape = new Rendering::DebugBox(glm::vec3(0.5f), COL_RGBA(0, 1, 1, 1));

                auto r = s.Create();
                return r.HasError() ? nullptr : r.Get();
            }

            case Physics::PhysicsShape::CAPSULE:
            {
                auto p = std::dynamic_pointer_cast<CapsuleParams>(params);
                if (!p) return nullptr;

                JPH::CapsuleShapeSettings s(p->halfHeight * scale.y, p->radius * glm::max(scale.x, scale.z));

                debugShape = new Rendering::DebugCapsule(0.5f, 0.5f, COL_RGBA(0, 1, 1, 1));

                auto r = s.Create();
                return r.HasError() ? nullptr : r.Get();
            }

            case Physics::PhysicsShape::CYLINDER:
            {
                auto p = std::dynamic_pointer_cast<CylinderParams>(params);
                if (!p) return nullptr;

                JPH::CylinderShapeSettings s(p->halfHeight * scale.y, p->radius * glm::max(scale.x, scale.z));

                debugShape = new Rendering::DebugCylinder(0.5f, 0.5f, COL_RGBA(0, 1, 1, 1));

                auto r = s.Create();
                return r.HasError() ? nullptr : r.Get();
            }

            default:
                return nullptr;
        }
    }

    void PhysicsBody::CreateBody(Physics::PhysicsShape shape, std::shared_ptr<ShapeParams> params, EMotionType motionType)
    {
        RemoveBody();

        mShape = nullptr;

        mShape = CreateJoltShape(shape, params);
        if (!mShape)
            return;

        this->shape = shape;
        this->params = params;
        this->motionType = motionType;

        glm::vec3 pos = parent->transform->GetPosition();
        glm::quat rot = parent->transform->GetRotation();

        JPH::BodyCreationSettings settings(
            mShape,
            JPH::RVec3(pos.x, pos.y, pos.z),
            JPH::Quat(rot.x, rot.y, rot.z, rot.w),
            motionType,
            motionType == EMotionType::Static
                ? Physics::Layers::NON_MOVING
                : Physics::Layers::MOVING
        );

        mBodyID = Core::GetEngine().GetPhysicsManager()->CreateBody(settings, this);
    }

    void PhysicsBody::ApplyTransformToPhysics(float dt)
    {
        auto& bi = Core::GetEngine().GetPhysicsManager()->GetBodyInterface();

        if (motionType == EMotionType::Static)
        {
            // Editor-only: recreate
            RemoveBody();
            CreateBody(shape, params, EMotionType::Static);
        }
        else if (motionType == EMotionType::Kinematic)
        {
            bi.MoveKinematic(
                mBodyID,
                ToJolt(parent->transform->GetPosition()),
                ToJolt(parent->transform->GetRotationQuat()),
                dt
            );
        }
        else // Dynamic (editor only)
        {
            bi.SetPositionAndRotation(
                mBodyID,
                ToJolt(parent->transform->GetPosition()),
                ToJolt(parent->transform->GetRotationQuat()),
                JPH::EActivation::Activate
            );
        }
    }

    void PhysicsBody::SyncTransformFromPhysics()
    {
        auto& bi = Core::GetEngine().GetPhysicsManager()->GetBodyInterface();

        auto pos = bi.GetCenterOfMassPosition(mBodyID);
        auto rot = bi.GetRotation(mBodyID);

        parent->transform->SetPosition(ToGLM(pos), false);
        parent->transform->SetRotation(ToGLM(rot), false);
    }

    void PhysicsBody::Tick(float dt){

        if(!activated)
            return;

        const bool playing = Core::GetEngine().IsInPlayMode();
        const bool posDirty   = parent->transform->IsDirty(DirtyFlags::Position);
        const bool rotDirty   = parent->transform->IsDirty(DirtyFlags::Rotation);
        const bool scaleDirty = parent->transform->IsDirty(DirtyFlags::Scale);

        if (!playing)
        {
            //EDITOR

            if (scaleDirty)
            {
                Update(shape, params, motionType, /*forceRecreation=*/true);
            }

            if (posDirty || rotDirty)
            {
                ApplyTransformToPhysics(dt);
            }
        }
        else
        {
            // GAME
            if (motionType == EMotionType::Dynamic)
            {
                SyncTransformFromPhysics();
            }
            else if (motionType == EMotionType::Kinematic && (posDirty || rotDirty))
            {
                ApplyTransformToPhysics(dt);
            }
        }

        parent->transform->ClearDirty(DirtyFlags::All);
    }

    void PhysicsBody::SetPosition(glm::vec3 newPos)
    {
        if (mBodyID.IsInvalid() || !activated)
            return;

        auto physics = Core::GetEngine().GetPhysicsManager();
        if (!physics)
            return;

        auto& bi = physics->GetPhysicsSystem().GetBodyInterface();

        JPH::RVec3 newPosition = ToJolt(newPos);

        
        if(motionType == EMotionType::Dynamic || motionType == EMotionType::Kinematic){
            // Teleport dynamic/kinematic body
            bi.SetPosition(
                mBodyID,
                newPosition,
                JPH::EActivation::Activate
            );
        }
        else{
            // Recreate the static body at a new pos
            CreateBody(shape, params, motionType);
        }
    }

    void PhysicsBody::SetRotation(glm::vec3 newRot)
    {
        if(!activated)
            return;
        
        SetRotation(glm::quat(newRot));
    }

    void PhysicsBody::SetRotation(glm::quat newRot)
    {
        
        if (mBodyID.IsInvalid() || !activated)
            return;

        auto physics = Core::GetEngine().GetPhysicsManager();
        if (!physics)
            return;

        auto& bi = physics->GetPhysicsSystem().GetBodyInterface();

        JPH::Quat joltRot = ToJolt(newRot);

        bi.SetRotation(
            mBodyID,
            joltRot,
            JPH::EActivation::Activate
        );
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