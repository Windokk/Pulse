#pragma once

#include "engine/projects/project.hpp"

#include "engine/filesystem/filesystem.hpp"

namespace Pulse::Engine::Serialization{

    std::shared_ptr<Projects::Project> DeserializeProject(const Filesystem::Path path);
    
}