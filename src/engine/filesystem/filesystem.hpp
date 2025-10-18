#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <algorithm>
#include <fstream>

#include <nlohmann/json.hpp>

#include "assetID.hpp"

namespace Epoch::Engine::Filesystem{

    enum class Type{
        T_IMAGE,
        T_SOUND,
        T_FONT,
        T_SHADER,
        T_TEXT,
        T_SCRIPT,
        T_LEVEL,
        T_MODEL,
        T_MATERIAL,
        T_CONFIG,
        T_DIRECTORY
    };

    // --- Path infos ---
    class Path {

        public:
        std::string full;  // Full/normalized path
    
        Path() = default;
        Path(const std::string &raw, bool normalize = false);

        // Normalize slashes, remove ../, etc.
        static std::string Normalize(const std::string& raw);
    
        Path RelativeTo(const Path& other) const;

        std::string GetExtensionString() const;

        std::string ReadFile() const;

        std::string WithoutExtension() const;

        bool WriteFile(const std::string &content) const;

        bool AppendToFile(const std::string &content) const;

        Type GetExtensionType() const;

        std::string GetAbsolutePath() const;

        std::string GetFilename(bool withExtension = true) const;
    
        std::string GetParent() const;

        int GetFileSize() const;

        Path GetParentPath() const;
    
        std::string GetParentArchive() const ;

        std::string GetPathInsideArchive() const;

        bool IsPacked() const;
    
        bool Exists() const;
    
        bool IsDirectory() const;

        static bool IsSubPathOf(const Path& child, const Path& parent);

        bool operator==(const Path& p) const {
            return full == p.full;
        }

        bool operator!=(const Path& p) const {
            return !(*this == p);
        }

        friend Path operator/(const Path& lhs, const Path& rhs);

        friend Path operator/(const std::string& lhs, const Path& rhs);

        friend Path operator/(const Path& lhs, const std::string& rhs);

    };

    // --- Basic file info ---
    struct FileInfo {
        Path path;
        std::string nameInProject = "";
        std::string extension = "";
        std::string name = "";
        Type type;
        bool isDirectory;
        int size;
    };

    struct AssetInfo{
        FileInfo baseInfos;
        std::vector<AssetID> dependencies;
    };

    class FileManager {
        public:

            Path GetCurrentExecutablePath();

            // File and directory browsing
            FileInfo GetFileInfos(const Path& path);
            void Init(Path projectResPath, Path engineResPath, Path projectRoot);
            std::vector<FileInfo> ListDirectory(const Path &path, std::vector<Type> acceptedExtensions, bool includeDirs = false, bool recursive = false);

            // Path tools
            bool IsPathInside(const Path& parent, const Path& child);
            bool HasExtension(const Path& path, const std::vector<std::string>& validExtensions);


            // Utility
            void SetEngineResRoot(const Path &path);
            void SetProjectResRoot(const Path &path);
            void SetProjectRoot(const Path& path);
            Path GetProjectRoot() { return projectRoot; };
            Path GetEngineResRoot() { return engineResPath; };
            Path GetProjectResRoot() { return projectResPath; };

        private:
            Path engineResPath;
            Path projectRoot;
            Path projectResPath;
    };
}