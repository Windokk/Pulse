#pragma once

#include <string>

#include "engine/filesystem/filesystem.hpp"

namespace Pulse::Engine::Rendering {

    std::string ResolveGLSLIncludes(const Filesystem::Path& path);

}
