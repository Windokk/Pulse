#pragma once

#include "audio_source.hpp"
#include "engine/core/reflection_fields.hpp"

// Reflection for class AudioSource

inline FieldInfo AudioSource_file_info = {
    "file",
    TypeID::String,
    offsetof(Pulse::Engine::ECS::Components::AudioSource, file),
    Editable,
    0, 0,
    nullptr,
    nullptr,
    &CopyConstruct<std::string>,
    &Assign<std::string>,
    &Destroy<std::string>,
    &Equals<std::string>
};

inline FieldInfo AudioSource_volume_info = {
    "volume",
    TypeID::Float,
    offsetof(Pulse::Engine::ECS::Components::AudioSource, volume),
    Editable,
    0, 0,
    nullptr,
    nullptr,
    &CopyConstruct<float>,
    &Assign<float>,
    &Destroy<float>,
    &Equals<float>
};

inline ClassDescriptor Pulse::Engine::ECS::Components::AudioSource::descriptor = {
    "AudioSource",
    {
        &AudioSource_file_info,
        &AudioSource_volume_info,
    }
};

