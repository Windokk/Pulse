#pragma once

#include "engine/filesystem/filesystem.hpp"

namespace Epoch::Engine::Serialization{

    void DeserializeAssetDataBase(const Filesystem::Path resourcesPath, const Filesystem::Path databasePath);

}