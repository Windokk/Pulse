#include "transform.hpp"
#include "engine/core/reflection_fields.hpp"

//Reflection for component : Transform

inline FieldInfo Transform_position_info = {
    "position",
    TypeID::Vec3,
    32,
    nullptr,
    WritePositionToTransform,
    nullptr,
    nullptr,
    Editable,
    0,0,
    nullptr,
    nullptr
};

inline FieldInfo Transform_rotation_info = {
    "rotation",
    TypeID::Vec3,
    44,
    ReadRotationFromTransform,
    WriteRotationToTransform,
    nullptr,
    nullptr,
    Editable,
    0,0,
    nullptr,
    nullptr
};

inline FieldInfo Transform_scale_info = {
    "scale",
    TypeID::Vec3,
    60,
    nullptr,
    WriteScaleToTransform,
    nullptr,
    nullptr,
    Editable,
    0,0,
    nullptr,
    nullptr
};

inline ComponentDescriptor Pulse::Engine::ECS::Components::Transform::descriptor = {
    "Transform",
    {
        &Transform_position_info,
        &Transform_rotation_info,
        &Transform_scale_info,
    }
};
