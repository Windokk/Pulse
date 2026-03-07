#pragma once

#include "character.hpp"
#include "engine/core/reflection_fields.hpp"

// Reflection for class Character

inline FieldInfo Character_vectorTest_info = {
    "vectorTest",
    TypeID::Mat3,
    offsetof(Character, vectorTest),
    Editable,
    0, 0,
    nullptr,
    nullptr,
    &CopyConstruct<glm::mat3>,
    &Assign<glm::mat3>,
    &Destroy<glm::mat3>,
    &Equals<glm::mat3>
};

inline FieldInfo Character_speed_info = {
    "speed",
    TypeID::Float,
    offsetof(Character, speed),
    Editable,
    0, 0,
    nullptr,
    nullptr,
    &CopyConstruct<float>,
    &Assign<float>,
    &Destroy<float>,
    &Equals<float>
};

inline FieldInfo Character_mouseSensitivity_info = {
    "mouseSensitivity",
    TypeID::Float,
    offsetof(Character, mouseSensitivity),
    Editable,
    0, 0,
    nullptr,
    nullptr,
    &CopyConstruct<float>,
    &Assign<float>,
    &Destroy<float>,
    &Equals<float>
};

inline FieldInfo Character_lockedMouseX_info = {
    "lockedMouseX",
    TypeID::Double,
    offsetof(Character, lockedMouseX),
    ReadOnly,
    0, 0,
    nullptr,
    nullptr,
    &CopyConstruct<double>,
    &Assign<double>,
    &Destroy<double>,
    &Equals<double>
};

inline FieldInfo Character_lockedMouseY_info = {
    "lockedMouseY",
    TypeID::Double,
    offsetof(Character, lockedMouseY),
    ReadOnly,
    0, 0,
    nullptr,
    nullptr,
    &CopyConstruct<double>,
    &Assign<double>,
    &Destroy<double>,
    &Equals<double>
};

inline FieldInfo Character_firstClick_info = {
    "firstClick",
    TypeID::Bool,
    offsetof(Character, firstClick),
    ReadOnly,
    0, 0,
    nullptr,
    nullptr,
    &CopyConstruct<bool>,
    &Assign<bool>,
    &Destroy<bool>,
    &Equals<bool>
};

inline FieldInfo Character_pitch_info = {
    "pitch",
    TypeID::Float,
    offsetof(Character, pitch),
    ReadOnly,
    0, 0,
    nullptr,
    nullptr,
    &CopyConstruct<float>,
    &Assign<float>,
    &Destroy<float>,
    &Equals<float>
};

inline FieldInfo Character_yaw_info = {
    "yaw",
    TypeID::Float,
    offsetof(Character, yaw),
    ReadOnly,
    0, 0,
    nullptr,
    nullptr,
    &CopyConstruct<float>,
    &Assign<float>,
    &Destroy<float>,
    &Equals<float>
};

inline ClassDescriptor Character::descriptor = {
    "Character",
    {
        &Character_vectorTest_info,
        &Character_speed_info,
        &Character_mouseSensitivity_info,
        &Character_lockedMouseX_info,
        &Character_lockedMouseY_info,
        &Character_firstClick_info,
        &Character_pitch_info,
        &Character_yaw_info,
    }
};

