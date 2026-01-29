#pragma once

#include "physics_body.hpp"
#include "engine/core/reflection_fields.hpp"

// Reflection for struct SphereParams

inline FieldInfo SphereParams_radius_info = {
    "radius",
    TypeID::Float,
    8,
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
    }
};

// Reflection for struct CapsuleParams

inline FieldInfo CapsuleParams_radius_info = {
    "radius",
    TypeID::Float,
    8,
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
    12,
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
    }
};

// Reflection for struct BoxParams

inline FieldInfo BoxParams_halfExtent_info = {
    "halfExtent",
    TypeID::Vec3,
    8,
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
    }
};

// Reflection for struct CylinderParams

inline FieldInfo CylinderParams_radius_info = {
    "radius",
    TypeID::Float,
    8,
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
    12,
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
    }
};

// Reflection for class PhysicsBody

static EnumDescriptor PhysicsShape_descriptor = {
    "PhysicsShape",
    {
        { "SPHERE", 0 },
        { "BOX", 1 },
        { "CAPSULE", 2 },
        { "CYLINDER", 3 },
    }
};

inline FieldInfo PhysicsBody_shape_info = {
    "shape",
    TypeID::Enum,
    36,
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
        { "Static", 0 },
        { "Kinematic", 1 },
        { "Dynamic", 2 },
    }
};

inline FieldInfo PhysicsBody_motionType_info = {
    "motionType",
    TypeID::Enum,
    64,
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
        &PhysicsBody_shape_info,
        &PhysicsBody_motionType_info,
    }
};

