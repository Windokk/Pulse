#pragma once

#include "engine/levels/level.hpp"

#include <string>


namespace Epoch::Engine::Serialization{

    std::shared_ptr<Levels::Level> DeserializeLevel(const std::string &pathInProject, const Filesystem::Path path);
}