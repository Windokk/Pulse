#pragma once

#include "physics_body.hpp"
#include "engine/core/reflection_fields.hpp"

// Reflection for struct SphereParams

inline FieldInfo SphereParams_radius_info = {
    "radius",
    TypeID::Float,
    offsetof(Pulse::Engine::ECS::Components::SphereParams, radius),
    Editable,
    0, 0,
    nullptr,
    nullptr,
    &CopyConstruct<float>,
    &Assign<float>,
    &Destroy<float>,
    &Equals<float>
};

inline StructDescriptor SphereParams_descriptor = {
    "SphereParams",
    {
        &SphereParams_radius_info,
    },
    sizeof(Pulse::Engine::ECS::Components::SphereParams),
    [](void* p) { new (p) Pulse::Engine::ECS::Components::SphereParams(); },
    [](void* p) { static_cast<Pulse::Engine::ECS::Components::SphereParams*>(p)->~SphereParams(); },
    [](void* d, const void* s) {
        *static_cast<Pulse::Engine::ECS::Components::SphereParams*>(d) =
        *static_cast<const Pulse::Engine::ECS::Components::SphereParams*>(s);
    },
    [](const void* a, const void* b) {
        return *static_cast<const Pulse::Engine::ECS::Components::SphereParams*>(a)
            == *static_cast<const Pulse::Engine::ECS::Components::SphereParams*>(b);
    }
};

// Reflection for struct CapsuleParams

inline FieldInfo CapsuleParams_radius_info = {
    "radius",
    TypeID::Float,
    offsetof(Pulse::Engine::ECS::Components::CapsuleParams, radius),
    Editable,
    0, 0,
    nullptr,
    nullptr,
    &CopyConstruct<float>,
    &Assign<float>,
    &Destroy<float>,
    &Equals<float>
};

inline FieldInfo CapsuleParams_halfHeight_info = {
    "halfHeight",
    TypeID::Float,
    offsetof(Pulse::Engine::ECS::Components::CapsuleParams, halfHeight),
    Editable,
    0, 0,
    nullptr,
    nullptr,
    &CopyConstruct<float>,
    &Assign<float>,
    &Destroy<float>,
    &Equals<float>
};

inline StructDescriptor CapsuleParams_descriptor = {
    "CapsuleParams",
    {
        &CapsuleParams_radius_info,
        &CapsuleParams_halfHeight_info,
    },
    sizeof(Pulse::Engine::ECS::Components::CapsuleParams),
    [](void* p) { new (p) Pulse::Engine::ECS::Components::CapsuleParams(); },
    [](void* p) { static_cast<Pulse::Engine::ECS::Components::CapsuleParams*>(p)->~CapsuleParams(); },
    [](void* d, const void* s) {
        *static_cast<Pulse::Engine::ECS::Components::CapsuleParams*>(d) =
        *static_cast<const Pulse::Engine::ECS::Components::CapsuleParams*>(s);
    },
    [](const void* a, const void* b) {
        return *static_cast<const Pulse::Engine::ECS::Components::CapsuleParams*>(a)
            == *static_cast<const Pulse::Engine::ECS::Components::CapsuleParams*>(b);
    }
};

// Reflection for struct BoxParams

inline FieldInfo BoxParams_halfExtent_info = {
    "halfExtent",
    TypeID::Vec3,
    offsetof(Pulse::Engine::ECS::Components::BoxParams, halfExtent),
    Editable,
    0, 0,
    nullptr,
    nullptr,
    &CopyConstruct<glm::vec3>,
    &Assign<glm::vec3>,
    &Destroy<glm::vec3>,
    &Equals<glm::vec3>
};

inline StructDescriptor BoxParams_descriptor = {
    "BoxParams",
    {
        &BoxParams_halfExtent_info,
    },
    sizeof(Pulse::Engine::ECS::Components::BoxParams),
    [](void* p) { new (p) Pulse::Engine::ECS::Components::BoxParams(); },
    [](void* p) { static_cast<Pulse::Engine::ECS::Components::BoxParams*>(p)->~BoxParams(); },
    [](void* d, const void* s) {
        *static_cast<Pulse::Engine::ECS::Components::BoxParams*>(d) =
        *static_cast<const Pulse::Engine::ECS::Components::BoxParams*>(s);
    },
    [](const void* a, const void* b) {
        return *static_cast<const Pulse::Engine::ECS::Components::BoxParams*>(a)
            == *static_cast<const Pulse::Engine::ECS::Components::BoxParams*>(b);
    }
};

// Reflection for struct CylinderParams

inline FieldInfo CylinderParams_radius_info = {
    "radius",
    TypeID::Float,
    offsetof(Pulse::Engine::ECS::Components::CylinderParams, radius),
    Editable,
    0, 0,
    nullptr,
    nullptr,
    &CopyConstruct<float>,
    &Assign<float>,
    &Destroy<float>,
    &Equals<float>
};

inline FieldInfo CylinderParams_halfHeight_info = {
    "halfHeight",
    TypeID::Float,
    offsetof(Pulse::Engine::ECS::Components::CylinderParams, halfHeight),
    Editable,
    0, 0,
    nullptr,
    nullptr,
    &CopyConstruct<float>,
    &Assign<float>,
    &Destroy<float>,
    &Equals<float>
};

inline StructDescriptor CylinderParams_descriptor = {
    "CylinderParams",
    {
        &CylinderParams_radius_info,
        &CylinderParams_halfHeight_info,
    },
    sizeof(Pulse::Engine::ECS::Components::CylinderParams),
    [](void* p) { new (p) Pulse::Engine::ECS::Components::CylinderParams(); },
    [](void* p) { static_cast<Pulse::Engine::ECS::Components::CylinderParams*>(p)->~CylinderParams(); },
    [](void* d, const void* s) {
        *static_cast<Pulse::Engine::ECS::Components::CylinderParams*>(d) =
        *static_cast<const Pulse::Engine::ECS::Components::CylinderParams*>(s);
    },
    [](const void* a, const void* b) {
        return *static_cast<const Pulse::Engine::ECS::Components::CylinderParams*>(a)
            == *static_cast<const Pulse::Engine::ECS::Components::CylinderParams*>(b);
    }
};

// Reflection for class PhysicsBody

inline FieldInfo PhysicsBody_params_info = {
    "params",
    TypeID::Struct,
    offsetof(Pulse::Engine::ECS::Components::PhysicsBody, params),
    Editable,
    0, 0,
    nullptr,
    nullptr,
    &CopyConstruct<InstancedStruct>,
    &Assign<InstancedStruct>,
    &Destroy<InstancedStruct>,
    &Equals<InstancedStruct>
};

static EnumDescriptor PhysicsShape_descriptor = {
    "PhysicsShape",
    {
        { 0, "SPHERE" },
        { 1, "BOX" },
        { 2, "CAPSULE" },
        { 3, "CYLINDER" },
    },
    sizeof(Physics::PhysicsShape)
};

inline FieldInfo PhysicsBody_shapeType_info = {
    "shapeType",
    TypeID::Enum,
    offsetof(Pulse::Engine::ECS::Components::PhysicsBody, shapeType),
    Editable,
    0, 0,
    nullptr,
    &PhysicsShape_descriptor,
    &CopyConstruct<Physics::PhysicsShape>,
    &Assign<Physics::PhysicsShape>,
    &Destroy<Physics::PhysicsShape>,
    &Equals<Physics::PhysicsShape>
};

static EnumDescriptor EMotionType_descriptor = {
    "EMotionType",
    {
        { 0, "Static" },
        { 1, "Kinematic" },
        { 2, "Dynamic" },
    },
    sizeof(EMotionType)
};

inline FieldInfo PhysicsBody_motionType_info = {
    "motionType",
    TypeID::Enum,
    offsetof(Pulse::Engine::ECS::Components::PhysicsBody, motionType),
    Editable,
    0, 0,
    nullptr,
    &EMotionType_descriptor,
    &CopyConstruct<EMotionType>,
    &Assign<EMotionType>,
    &Destroy<EMotionType>,
    &Equals<EMotionType>
};

inline ClassDescriptor Pulse::Engine::ECS::Components::PhysicsBody::descriptor = {
    "PhysicsBody",
    {
        &PhysicsBody_params_info,
        &PhysicsBody_shapeType_info,
        &PhysicsBody_motionType_info,
    }
};

