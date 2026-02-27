#include "asset_browser.hpp"

#include "editor/gui/main_window.hpp"

namespace Pulse::Editor::GUI{

    void AssetBrowser::Refresh()
    {
        items.clear();

        auto* fileManager = Engine::Core::GetEngine().GetFileManager();

        auto files = fileManager->ListDirectory(
            currentPath,
            allowedTypes,
            true,
            false
        );

        items.reserve(files.size());

        auto atlas = EditorResources::Instance().GetIconAtlas();

        for (auto& file : files)
        {
            BrowserItem item;
            item.path = file.path;
            item.type = file.type;
            item.isDirectory = file.isDirectory;

            item.icon = &atlas->GetRegion(
                file.isDirectory ?
                Engine::Filesystem::Type::T_DIRECTORY :
                file.type
            );

            items.push_back(std::move(item));
        }

        dirty = false;
    }

    void AssetBrowser::Draw()
    {
        if (dirty)
            Refresh();

        ImGui::Begin("Asset Browser");

        DrawBreadcrumb();
        ImGui::Separator();

        DrawAssets();

        ImGui::End();
    }

    void AssetBrowser::DrawBreadcrumb()
    {
        auto& engine = Engine::Core::GetEngine();

        std::string projectRoot =
            engine.GetCurrentProject()->GetProjectResourcesPath().full;

        std::string relative = Engine::Filesystem::Path(projectRoot).RelativeTo(currentPath).full;

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 0));

        if (ImGui::Button(engine.GetCurrentProject()
                            ->GetProjectResourcesPath()
                            .GetFilename()
                            .c_str()))
        {
            NavigateTo(projectRoot);
        }

        if (!relative.empty())
        {
            std::stringstream ss(relative);
            std::string segment;
            std::string accum = projectRoot;

            while (std::getline(ss, segment, '/'))
            {
                ImGui::SameLine();
                ImGui::Text("/");
                ImGui::SameLine();

                accum += "/" + segment;

                if (ImGui::Button(segment.c_str()))
                    NavigateTo(accum);
            }
        }

        ImGui::PopStyleVar();
    }

    void AssetBrowser::DrawAssets()
    {
        float panelWidth = ImGui::GetContentRegionAvail().x;
        int columnCount = std::max(1, (int)(panelWidth / (thumbnailSize + 20)));

        if (ImGui::BeginTable("Assets", columnCount))
        {
            ImGuiListClipper clipper;
            clipper.Begin(items.size());

            while (clipper.Step())
            {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                {
                    ImGui::TableNextColumn();
                    DrawItem(i);
                }
            }

            ImGui::EndTable();
        }
    }

    void AssetBrowser::DrawItem(int index)
    {
        auto& item = items[index];

        ImGui::PushID(index);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
        ImGui::ImageButton(
            "##thumb",
            (void*)(intptr_t)EditorResources::Instance()
                .GetIconAtlas()
                ->GetTexture()
                ->GetID(),
            ImVec2(thumbnailSize, thumbnailSize),
            item.icon->uv0,
            item.icon->uv1
        );
        ImGui::PopStyleColor();

        if (ImGui::IsItemHovered() &&
            ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            if (item.isDirectory)
                NavigateTo(item.path.full);
        }

        ImGui::TextWrapped("%s", item.path.GetFilename().c_str());

        ImGui::PopID();
    }

    void AssetBrowser::RenameAsset(const std::string& oldPath, const std::string& newName)
    {
        if (newName.empty())
            return;

        auto& engine = Engine::Core::GetEngine();
        auto* fileManager = engine.GetFileManager();

        Engine::Filesystem::Path oldFile(oldPath);

        if (!oldFile.Exists())
            return;

        std::string cleanName = newName;
        std::replace(cleanName.begin(), cleanName.end(), '\\', '_');
        std::replace(cleanName.begin(), cleanName.end(), '/', '_');

        std::string extension = "";
        if (!oldFile.IsDirectory())
            extension = oldFile.GetExtensionString();

        std::string finalName = cleanName;

        if (!extension.empty())
        {
            if (cleanName.find(extension) == std::string::npos)
                finalName += "." + extension;
        }

        Engine::Filesystem::Path newPath =
            Engine::Filesystem::Path(oldFile.GetParent()) / finalName;

        if (newPath.Exists())
            return;

        fileManager->RenameFile(oldFile, newPath);

        auto* assetManager = engine.GetAssetIDManager();
        if (assetManager)
            /// @todo recreate assets database, resources, etc...
            //assetManager->OnAssetRenamed(oldFile, newPath);

        renamingID = {};
        renameBuffer[0] = '\0';
    }

    void AssetBrowser::NavigateTo(const std::string& path)
    {
        Engine::Filesystem::Path target(path);

        if (!target.Exists() || !target.IsDirectory())
            return;

        currentPath = target;
        dirty = true; // mark for refresh
    }
}