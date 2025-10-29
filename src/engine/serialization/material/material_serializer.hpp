#pragma once

#include "engine/rendering/material/material.hpp"

#include "engine/filesystem/filesystem.hpp"

namespace Pulse::Engine::Serialization{

    std::shared_ptr<Rendering::Material> DeserializeMaterial(const Filesystem::Path path);

}