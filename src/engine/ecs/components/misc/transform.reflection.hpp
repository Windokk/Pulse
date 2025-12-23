#include "transform.hpp"
#include "engine/core/reflection_types.hpp"

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
    
};

ComponentDescriptor Transform_descriptor = {
    "Transform",
    {
        &position_info,
        &rotation_info,
        &scale_info,
    }
};
