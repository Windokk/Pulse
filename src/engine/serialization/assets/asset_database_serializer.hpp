#pragma once

#include "engine/filesystem/filesystem.hpp"

namespace Pulse::Engine::Serialization{

    void DeserializeAssetDataBase(const Filesystem::Path resourcesPath, const Filesystem::Path databasePath);
    void SerializeAssetDataBase(const Filesystem::Path databasePath);
}