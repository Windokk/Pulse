#include "physics_body.hpp"
#include "engine/core/reflection_fields.hpp"

//Reflection for component : PhysicsBody

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
    TypeID::Unknown,
    36,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    Editable,
    0,0,
    nullptr,
    &PhysicsShape_descriptor
};

inline FieldInfo PhysicsBody_scale_info = {
    "scale",
    TypeID::Vec3,
    40,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    Editable,
    0,0,
    nullptr,
    nullptr
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
    TypeID::Unknown,
    64,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    Editable,
    0,0,
    nullptr,
    &EMotionType_descriptor
};

inline ComponentDescriptor Pulse::Engine::ECS::Components::PhysicsBody::descriptor = {
    "PhysicsBody",
    {
        &PhysicsBody_shape_info,
        &PhysicsBody_scale_info,
        &PhysicsBody_motionType_info,
    }
};
