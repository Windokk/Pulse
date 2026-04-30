#include "physics_body.hpp"

#include "engine/ecs/objects/actors/actor.hpp"
#include "engine/core/engine.hpp"

#include "physics_body.reflection.hpp"

#include <iostream>

#include <thread>

namespace Pulse::Engine::ECS::Components{
    PhysicsBody::PhysicsBody(std::shared_ptr<Objects::Actor> parent, uint32_t local_id) : Component(parent, local_id)
    {
        params.Initialize(&BoxParams_descriptor);
    }

    void PhysicsBody::Update(const Physics::PhysicsShape& newShape, const InstancedStruct& newParams, EMotionType newMotionType, bool forceRecreation)
    {
        auto physics = Core::GetEngine().GetPhysicsManager();
        if (!physics || mBodyID.IsInvalid())
            return;

        auto& bi = physics->GetBodyInterface();

        // --- Shape update ---
        if (newShape != shapeType || newParams != params || forceRecreation)
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
            shapeType = newShape;
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

    JPH::ShapeRefC PhysicsBody::CreateJoltShape(Physics::PhysicsShape shape, const InstancedStruct& params)
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
                auto p = reinterpret_cast<const SphereParams*>(params.data);
                if (!p) return nullptr;

                float size = std::max({ scale.x, scale.y, scale.z });

                JPH::SphereShapeSettings s(p->radius * size);

                debugShape = new Rendering::DebugSphere(p->radius, COL_RGBA(0, 1, 1, 1));

                auto r = s.Create();
                return r.HasError() ? nullptr : r.Get();
            }

            case Physics::PhysicsShape::BOX:
            {
                auto p = reinterpret_cast<const BoxParams*>(params.data);
                if (!p) return nullptr;

                JPH::BoxShapeSettings s(
                    JPH::Vec3(p->halfExtent.x * scale.x, p->halfExtent.y * scale.y, p->halfExtent.z * scale.z)
                );

                debugShape = new Rendering::DebugBox(p->halfExtent, COL_RGBA(0, 1, 1, 1));

                auto r = s.Create();
                return r.HasError() ? nullptr : r.Get();
            }

            case Physics::PhysicsShape::CAPSULE:
            {
                auto p = reinterpret_cast<const CapsuleParams*>(params.data);
                if (!p) return nullptr;

                JPH::CapsuleShapeSettings s(p->halfHeight * scale.y, p->radius * glm::max(scale.x, scale.z));

                debugShape = new Rendering::DebugCapsule(p->radius, p->halfHeight, COL_RGBA(0, 1, 1, 1));

                auto r = s.Create();
                return r.HasError() ? nullptr : r.Get();
            }

            case Physics::PhysicsShape::CYLINDER:
            {
                auto p = reinterpret_cast<const CylinderParams*>(params.data);
                if (!p) return nullptr;

                JPH::CylinderShapeSettings s(p->halfHeight * scale.y, p->radius * glm::max(scale.x, scale.z));

                debugShape = new Rendering::DebugCylinder(p->radius, p->halfHeight, COL_RGBA(0, 1, 1, 1));

                auto r = s.Create();
                return r.HasError() ? nullptr : r.Get();
            }

            default:
                return nullptr;
        }
    }

    void PhysicsBody::CreateBody(Physics::PhysicsShape shape, const InstancedStruct& params, EMotionType motionType)
    {
        RemoveBody();

        mShape = nullptr;

        mShape = CreateJoltShape(shape, params);
        if (!mShape)
            return;

        this->shapeType = shape;
        this->params = params;
        this->motionType = motionType;

        JPH::BodyCreationSettings settings(
            mShape,
            ToJolt(parent->transform->GetPosition()),
            ToJolt(parent->transform->GetRotationQuat()),
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
            CreateBody(shapeType, params, EMotionType::Static);
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

        if(shouldUpdateShape)
        {
            Update(shapeType, params, motionType, true);
            shouldUpdateShape = false;
        }

        if (!playing)
        {
            //EDITOR

            if (scaleDirty)
            {
                Update(shapeType, params, motionType, /*forceRecreation=*/true);
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
            CreateBody(shapeType, params, motionType);
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

    void PhysicsBody::Deserialize(const json componentData)
    {
        auto getString = [&](const char* key) -> std::optional<std::string>
        {
            if (!componentData.contains(key) || !componentData[key].is_string())
                return std::nullopt;
            return componentData[key].get<std::string>();
        };

        auto getBool = [&](const char* key, bool defaultValue = false) -> bool
        {
            if (!componentData.contains(key) || !componentData[key].is_boolean())
                return defaultValue;
            return componentData[key].get<bool>();
        };

        const auto& parentName = parent ? parent->GetName() : "UNKNOWN";

        // ---------------- SHAPE ----------------
        auto shapeStr = getString("shape");
        if (!shapeStr)
        {
            DEBUG_ERROR("Missing physics shape for actor: " + (std::string)parentName);
            return;
        }

        Physics::PhysicsShape shape;

        if (*shapeStr == "box") shape = Physics::PhysicsShape::BOX;
        else if (*shapeStr == "sphere") shape = Physics::PhysicsShape::SPHERE;
        else if (*shapeStr == "capsule") shape = Physics::PhysicsShape::CAPSULE;
        else if (*shapeStr == "cylinder") shape = Physics::PhysicsShape::CYLINDER;
        else
        {
            DEBUG_ERROR("Unknown physics shape: " + *shapeStr);
            return;
        }

        // ---------------- MOTION ----------------
        auto motionStr = getString("motion_type");
        if (!motionStr)
        {
            DEBUG_ERROR("Missing motion type for actor: " + (std::string)parentName);
            return;
        }

        JPH::EMotionType motion;

        if (*motionStr == "dynamic") motion = JPH::EMotionType::Dynamic;
        else if (*motionStr == "kinematic") motion = JPH::EMotionType::Kinematic;
        else if (*motionStr == "static") motion = JPH::EMotionType::Static;
        else
        {
            DEBUG_ERROR("Unknown motion type: " + *motionStr);
            return;
        }

        // ---------------- PARAMS ----------------
        if (!componentData.contains("params") || !componentData["params"].is_object())
        {
            DEBUG_ERROR("Missing physics params for actor: " + (std::string)parentName);
            return;
        }

        const auto& p = componentData["params"];
        InstancedStruct params;

        auto requireFloat = [&](const char* key, std::optional<float>& out) -> bool
        {
            if (!p.contains(key) || !p[key].is_number())
                return false;
            out = p[key].get<float>();
            return true;
        };

        switch (shape)
        {
            case Physics::PhysicsShape::BOX:
            {
                std::optional<float> x, y, z;
                if (!requireFloat("x", x) || !requireFloat("y", y) || !requireFloat("z", z))
                {
                    DEBUG_ERROR("Invalid BOX params for actor: " + (std::string)parentName);
                    return;
                }

                params.Initialize(&BoxParams_descriptor);
                auto& vec = *reinterpret_cast<glm::vec3*>(params.data);
                vec = glm::vec3(*x, *y, *z);
                break;
            }

            case Physics::PhysicsShape::SPHERE:
            {
                std::optional<float> radius;
                if (!requireFloat("radius", radius))
                {
                    DEBUG_ERROR("Invalid SPHERE params for actor: " + (std::string)parentName);
                    return;
                }

                params.Initialize(&SphereParams_descriptor);
                *reinterpret_cast<float*>(params.data) = *radius;
                break;
            }

            case Physics::PhysicsShape::CAPSULE:
            {
                std::optional<float> radius, halfHeight;
                if (!requireFloat("radius", radius) || !requireFloat("halfHeight", halfHeight))
                {
                    DEBUG_ERROR("Invalid CAPSULE params for actor: " + (std::string)parentName);
                    return;
                }

                params.Initialize(&CapsuleParams_descriptor);
                auto* dataPtr = reinterpret_cast<CapsuleParams*>(params.data);
                dataPtr->radius = *radius;
                dataPtr->halfHeight = *halfHeight;
                break;
            }

            case Physics::PhysicsShape::CYLINDER:
            {
                std::optional<float> radius, halfHeight;
                if (!requireFloat("radius", radius) || !requireFloat("halfHeight", halfHeight))
                {
                    DEBUG_ERROR("Invalid CYLINDER params for actor: " + (std::string)parentName);
                    return;
                }

                params.Initialize(&CylinderParams_descriptor);
                auto* dataPtr = reinterpret_cast<CylinderParams*>(params.data);
                dataPtr->radius = *radius;
                dataPtr->halfHeight = *halfHeight;
                break;
            }

            default:
                DEBUG_ERROR("Unhandled physics shape for actor: " + (std::string)parentName);
                return;
        }

        // ---------------- CREATE BODY ----------------
        CreateBody(shape, params, motion);

        // ---------------- ACTIVE STATE ----------------
        Activate();
        if (componentData.contains("active") && componentData["active"].is_boolean())
        {
            if (!componentData["active"].get<bool>())
                DeActivate();
        }
    }

    ordered_json PhysicsBody::Serialize()
    {
        ordered_json comp;

        comp["type"] = "physics_body";

        comp["active"] = activated;

        switch (shapeType)
        {
            case Physics::BOX: {
                comp["shape"] = "box";
                auto boxParams = reinterpret_cast<const BoxParams*>(params.data);
                comp["params"]["x"] = boxParams->halfExtent.x;
                comp["params"]["y"] = boxParams->halfExtent.y;
                comp["params"]["z"] = boxParams->halfExtent.z;
                break;
            }

            case Physics::SPHERE: {
                comp["shape"] = "sphere";
                auto sphereParams = reinterpret_cast<const SphereParams*>(params.data);
                comp["params"]["radius"] = sphereParams->radius;
                break;
            }

            case Physics::CAPSULE: {
                comp["shape"] = "capsule";
                auto capsuleParams = reinterpret_cast<const CapsuleParams*>(params.data);
                comp["params"]["radius"] = capsuleParams->radius;
                comp["params"]["halfHeight"] = capsuleParams->halfHeight;
                break;
            }

            case Physics::CYLINDER: {
                comp["shape"] = "cylinder";
                auto cylinderParams = reinterpret_cast<const CylinderParams*>(params.data);
                comp["params"]["radius"] = cylinderParams->radius;
                comp["params"]["halfHeight"] = cylinderParams->halfHeight;
                break;
            }

            default:
                DEBUG_ERROR("Unknown physics shape for actor: " + std::string(parent->GetName()));
                break;
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
        auto cloned = Object::Create<PhysicsBody>(*this);
        cloned->mBodyID = JPH::BodyID();
        cloned->debugShape = nullptr;
        cloned->CreateBody(shapeType, params, motionType);
        return cloned;
    }

    void PhysicsBody::ForceShapeUpdate(const Physics::PhysicsShape &shape, const InstancedStruct& params, EMotionType motionType)
    {
        this->shapeType = shape;
        this->params = params;
        this->motionType = motionType;

        this->shouldUpdateShape = true;
    }

    void PhysicsBody::OnFieldChanged(const FieldChangedEvent& event){
        if(event.field->name == "shapeType"){

            switch (shapeType)
            {
                case Physics::BOX:
                    params.Initialize(&BoxParams_descriptor);
                    break;

                case Physics::SPHERE:
                    params.Initialize(&SphereParams_descriptor);
                    break;

                case Physics::CAPSULE:
                    params.Initialize(&CapsuleParams_descriptor);
                    break;

                case Physics::CYLINDER:
                    params.Initialize(&CylinderParams_descriptor);
                    break;
            }

            ForceShapeUpdate(shapeType, params, GetMotionType());
        }
        else if(event.field->name == "motionType"){
            ForceShapeUpdate(GetShapeType(), params, motionType);
        }
        else{
            ForceShapeUpdate(shapeType, params, motionType);
        }
    }
}