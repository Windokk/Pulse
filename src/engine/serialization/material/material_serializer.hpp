#pragma once

#include "engine/filesystem/filesystem.hpp"

namespace Pulse::Engine::Rendering{
    class Material;
}

namespace Pulse::Engine::Serialization{

    std::shared_ptr<Rendering::Material> DeserializeMaterial(const Filesystem::Path path);

}