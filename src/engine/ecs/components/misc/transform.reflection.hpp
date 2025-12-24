#include "transform.hpp"
#include "engine/core/reflection_fields.hpp"

//Reflection for component : Transform

FieldInfo position_info = {
    "position",
    TypeID::Vec3,
    32,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    Editable,
    0,0,
    nullptr,
    nullptr
};

FieldInfo rotation_info = {
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

FieldInfo scale_info = {
    "scale",
    TypeID::Vec3,
    60,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    Editable,
    0,0,
    nullptr,
    nullptr
};

ComponentDescriptor Transform_descriptor = {
    "Transform",
    {
        &position_info,
        &rotation_info,
        &scale_info,
    }
};
