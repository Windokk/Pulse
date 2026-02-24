#pragma once

#include "physics_body.hpp"
#include "engine/core/reflection_fields.hpp"

// Reflection for struct SphereParams

inline FieldInfo SphereParams_radius_info = {
    "radius",
    TypeID::Float,
    offsetof(Pulse::Engine::ECS::Components::SphereParams, radius),
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    Editable,
    0, 0,
    nullptr,
    nullptr
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
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    Editable,
    0, 0,
    nullptr,
    nullptr
};

inline FieldInfo CapsuleParams_halfHeight_info = {
    "halfHeight",
    TypeID::Float,
    offsetof(Pulse::Engine::ECS::Components::CapsuleParams, halfHeight),
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    Editable,
    0, 0,
    nullptr,
    nullptr
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
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    Editable,
    0, 0,
    nullptr,
    nullptr
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
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    Editable,
    0, 0,
    nullptr,
    nullptr
};

inline FieldInfo CylinderParams_halfHeight_info = {
    "halfHeight",
    TypeID::Float,
    offsetof(Pulse::Engine::ECS::Components::CylinderParams, halfHeight),
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    Editable,
    0, 0,
    nullptr,
    nullptr
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
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    Editable,
    0, 0,
    nullptr,
    nullptr
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

inline FieldInfo PhysicsBody_shape_info = {
    "shape",
    TypeID::Enum,
    offsetof(Pulse::Engine::ECS::Components::PhysicsBody, shape),
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    Editable,
    0, 0,
    nullptr,
    &PhysicsShape_descriptor
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
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    Editable,
    0, 0,
    nullptr,
    &EMotionType_descriptor
};

inline ClassDescriptor Pulse::Engine::ECS::Components::PhysicsBody::descriptor = {
    "PhysicsBody",
    {
        &PhysicsBody_params_info,
        &PhysicsBody_shape_info,
        &PhysicsBody_motionType_info,
    }
};

