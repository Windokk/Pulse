#pragma once

#include "engine/core/engine.hpp"

#include "editor/gui/resources/editor_resources.hpp"

namespace Pulse::Editor::GUI{

    struct BrowserItem
    {
        Engine::Filesystem::Path path;
        Engine::Filesystem::Type type;
        bool isDirectory;

        const AtlasRegion* icon;  // cached pointer
    };

    class AssetBrowser
    {
        public:
            void Refresh();
            void Draw();
            void NavigateTo(const std::string& path);

        private:
            void DrawBreadcrumb();
            void DrawAssets();
            void DrawItem(int index);
            void RenameAsset(const std::string &oldPath, const std::string &newName);

            Engine::Filesystem::Path currentPath;
            float thumbnailSize = 72.f;

            int renamingID = -1;
            char renameBuffer[256]{};

            std::vector<BrowserItem> items;
            bool dirty = true;

            std::vector<Engine::Filesystem::Type> allowedTypes = {
                Engine::Filesystem::Type::T_IMAGE,
                Engine::Filesystem::Type::T_SOUND,
                Engine::Filesystem::Type::T_FONT,
                Engine::Filesystem::Type::T_SHADER,
                Engine::Filesystem::Type::T_TEXT,
                Engine::Filesystem::Type::T_SCRIPT,
                Engine::Filesystem::Type::T_LEVEL,
                Engine::Filesystem::Type::T_MODEL,
                Engine::Filesystem::Type::T_MATERIAL,
                Engine::Filesystem::Type::T_CONFIG,
                Engine::Filesystem::Type::T_DIRECTORY
            };
    };
}