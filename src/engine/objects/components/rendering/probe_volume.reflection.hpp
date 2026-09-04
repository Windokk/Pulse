#pragma once

#include "probe_volume.hpp"
#include "engine/core/reflection_fields.hpp"

// Reflection for class ProbeVolume

inline FieldInfo ProbeVolume_halfExtent_info = {
    "halfExtent",
    TypeID::Vec3,
    offsetof(Pulse::Engine::Objects::Components::ProbeVolume, halfExtent),
    Editable,
    0, 0,
    nullptr,
    nullptr,
    &CopyConstruct<glm::vec3>,
    &Assign<glm::vec3>,
    &Destroy<glm::vec3>,
    &Equals<glm::vec3>
};

inline FieldInfo ProbeVolume_probeCounts_info = {
    "probeCounts",
    TypeID::IVec3,
    offsetof(Pulse::Engine::Objects::Components::ProbeVolume, probeCounts),
    Editable,
    0, 0,
    nullptr,
    nullptr,
    &CopyConstruct<glm::ivec3>,
    &Assign<glm::ivec3>,
    &Destroy<glm::ivec3>,
    &Equals<glm::ivec3>
};

inline FieldInfo ProbeVolume_raysPerProbe_info = {
    "raysPerProbe",
    TypeID::Int32,
    offsetof(Pulse::Engine::Objects::Components::ProbeVolume, raysPerProbe),
    Editable,
    0, 0,
    nullptr,
    nullptr,
    &CopyConstruct<int>,
    &Assign<int>,
    &Destroy<int>,
    &Equals<int>
};

inline FieldInfo ProbeVolume_maxBounces_info = {
    "maxBounces",
    TypeID::Int32,
    offsetof(Pulse::Engine::Objects::Components::ProbeVolume, maxBounces),
    Editable,
    0, 0,
    nullptr,
    nullptr,
    &CopyConstruct<int>,
    &Assign<int>,
    &Destroy<int>,
    &Equals<int>
};

inline ClassDescriptor Pulse::Engine::Objects::Components::ProbeVolume::descriptor = {
    "ProbeVolume",
    {
        &ProbeVolume_halfExtent_info,
        &ProbeVolume_probeCounts_info,
        &ProbeVolume_raysPerProbe_info,
        &ProbeVolume_maxBounces_info,
    }
};
