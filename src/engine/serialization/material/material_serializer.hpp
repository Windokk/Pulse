#pragma once

#include "engine/rendering/material/material.hpp"

#include "engine/filesystem/filesystem.hpp"

namespace Epoch::Engine::Serialization{

    std::shared_ptr<Rendering::Material> DeserializeMaterial(const Filesystem::Path path);

}