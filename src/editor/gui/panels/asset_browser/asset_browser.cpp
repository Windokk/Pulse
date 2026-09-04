#include "asset_browser.hpp"

#include "editor/gui/main_window.hpp"

#include "engine/projects/project.hpp"
#include "engine/levels/level_manager.hpp"
#include "engine/levels/level.hpp"
#include "engine/core/resources/resources_manager.hpp"
#include "engine/rendering/renderer/renderer.hpp"

namespace Pulse::Editor::GUI{

    const ImGuiTableSortSpecs* Asset::s_current_sort_specs = NULL;

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

        auto atlas = EditorResources::Instance().GetIconAtlas();

        for (int i = 0; i < files.size(); i++)
        {
            auto file = files[i];

            items.push_back(Asset(
                ImHashStr(file.path.full.c_str()), 
                file.path,
                file.type,
                file.isDirectory,
                &atlas->GetRegion(file.isDirectory ? Engine::Filesystem::Type::T_DIRECTORY : file.type)
            ));
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
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowContentSize(ImVec2(0.0f, LayoutOuterPadding + LayoutLineCount * (LayoutItemSize.y + LayoutItemSpacing)));
        if (ImGui::BeginChild("Assets", ImVec2(0.0f, -ImGui::GetTextLineHeightWithSpacing()), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoMove))
        {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();

            const float avail_width = ImGui::GetContentRegionAvail().x;
            UpdateLayoutSizes(avail_width);

            // Calculate and store start position.
            ImVec2 start_pos = ImGui::GetCursorScreenPos();
            start_pos = ImVec2(start_pos.x + LayoutOuterPadding, start_pos.y + LayoutOuterPadding);
            ImGui::SetCursorScreenPos(start_pos);

            // Multi-select
            ImGuiMultiSelectFlags ms_flags = ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_ClearOnClickVoid;

            // - Enable box-select (in 2D mode, so that changing box-select rectangle X1/X2 boundaries will affect clipped items)
            if (AllowBoxSelect)
                ms_flags |= ImGuiMultiSelectFlags_BoxSelect2d;

            // - This feature allows dragging an unselected item without selecting it (rarely used)
            if (AllowDragUnselected)
                ms_flags |= ImGuiMultiSelectFlags_SelectOnClickRelease;

            // - Enable keyboard wrapping on X axis
            // (FIXME-MULTISELECT: We haven't designed/exposed a general nav wrapping api yet, so this flag is provided as a courtesy to avoid doing:
            //    ImGui::NavMoveRequestTryWrapping(ImGui::GetCurrentWindow(), ImGuiNavMoveFlags_WrapX);
            // When we finish implementing a more general API for this, we will obsolete this flag in favor of the new system)
            ms_flags |= ImGuiMultiSelectFlags_NavWrapX;

            ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(ms_flags, selection.Size, items.size());

            // Use custom selection adapter: store ID in selection (recommended)
            selection.UserData = this;
            selection.AdapterIndexToStorageId = [](ImGuiSelectionBasicStorage* self_, int idx) { AssetBrowser* self = (AssetBrowser*)self_->UserData; return self->items[idx].id; };
            selection.ApplyRequests(ms_io);

            const bool want_delete = (ImGui::Shortcut(ImGuiKey_Delete, ImGuiInputFlags_Repeat) && (selection.Size > 0)) || requestDelete;
            const int item_curr_idx_to_focus = want_delete ? selection.ApplyDeletionPreLoop(ms_io, items.size()) : -1;
            requestDelete = false;

            // Push LayoutSelectableSpacing (which is LayoutItemSpacing minus hit-spacing, if we decide to have hit gaps between items)
            // Altering style ItemSpacing may seem unnecessary as we position every items using SetCursorScreenPos()...
            // But it is necessary for two reasons:
            // - Selectables uses it by default to visually fill the space between two items.
            // - The vertical spacing would be measured by Clipper to calculate line height if we didn't provide it explicitly (here we do).
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(LayoutSelectableSpacing, LayoutSelectableSpacing));

            // Rendering parameters
            const ImU32 icon_type_overlay_colors[3] = { 0, IM_COL32(200, 70, 70, 255), IM_COL32(70, 170, 70, 255) };
            const ImU32 icon_bg_color = ImGui::GetColorU32(IM_COL32(35, 35, 35, 220));
            const ImVec2 icon_type_overlay_size = ImVec2(4.0f, 4.0f);
            const bool display_label = (LayoutItemSize.x >= ImGui::CalcTextSize("999").x);

            const int column_count = LayoutColumnCount;
            ImGuiListClipper clipper;
            clipper.Begin(LayoutLineCount, LayoutItemStep.y);
            if (item_curr_idx_to_focus != -1)
                clipper.IncludeItemByIndex(item_curr_idx_to_focus / column_count); // Ensure focused item line is not clipped.
            if (ms_io->RangeSrcItem != -1)
                clipper.IncludeItemByIndex((int)ms_io->RangeSrcItem / column_count); // Ensure RangeSrc item line is not clipped.
            while (clipper.Step())
            {
                for (int line_idx = clipper.DisplayStart; line_idx < clipper.DisplayEnd; line_idx++)
                {
                    const int item_min_idx_for_current_line = line_idx * column_count;
                    const int item_max_idx_for_current_line = IM_MIN((line_idx + 1) * column_count, items.size());
                    for (int item_idx = item_min_idx_for_current_line; item_idx < item_max_idx_for_current_line; ++item_idx)
                    {
                        Asset* item_data = &items[item_idx];
                        ImGui::PushID((int)item_data->id);

                        // Position item
                        ImVec2 pos = ImVec2(start_pos.x + (item_idx % column_count) * LayoutItemStep.x, start_pos.y + line_idx * LayoutItemStep.y);
                        ImGui::SetCursorScreenPos(pos);

                        ImGui::SetNextItemSelectionUserData(item_idx);
                        bool item_is_selected = selection.Contains((ImGuiID)item_data->id);
                        bool item_is_visible = ImGui::IsRectVisible(LayoutItemSize);
                        ImGui::Selectable("", item_is_selected, ImGuiSelectableFlags_None, LayoutItemSize);

                        if (!item_data->isDirectory && item_data->type == Engine::Filesystem::Type::T_LEVEL
                            && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                        {
                            RequestOpenLevel(item_data->path);
                        }

                        // Update our selection state immediately (without waiting for EndMultiSelect() requests)
                        // because we use this to alter the color of our text/icon.
                        if (ImGui::IsItemToggledSelection())
                            item_is_selected = !item_is_selected;

                        // Focus (for after deletion)
                        if (item_curr_idx_to_focus == item_idx)
                            ImGui::SetKeyboardFocusHere(-1);

                        // Drag and drop
                        if (ImGui::BeginDragDropSource())
                        {
                            // Create payload with full selection OR single unselected item.
                            // (the later is only possible when using ImGuiMultiSelectFlags_SelectOnClickRelease)
                            if (ImGui::GetDragDropPayload() == NULL)
                            {
                                ImVector<ImGuiID> payload_items;
                                void* it = NULL;
                                ImGuiID id = 0;
                                if (!item_is_selected)
                                    payload_items.push_back(item_data->id);
                                else
                                    while (selection.GetNextSelectedItem(&it, &id))
                                        payload_items.push_back(id);
                                ImGui::SetDragDropPayload("ASSETS_BROWSER_ITEMS", payload_items.Data, (size_t)payload_items.size_in_bytes());
                            }

                            // Display payload content in tooltip, by extracting it from the payload data
                            // (we could read from selection, but it is more correct and reusable to read from payload)
                            const ImGuiPayload* payload = ImGui::GetDragDropPayload();
                            const int payload_count = (int)payload->DataSize / (int)sizeof(ImGuiID);
                            ImGui::Text("%d assets", payload_count);

                            ImGui::EndDragDropSource();
                        }

                        // Render icon (a real app would likely display an image/thumbnail here)
                        if (item_is_visible)
                        {
                            ImVec2 box_min(pos.x - 1, pos.y - 1);
                            ImVec2 box_max(box_min.x + LayoutItemSize.x + 2, box_min.y + LayoutItemSize.y + 2); // Dubious
                            draw_list->AddRectFilled(box_min, box_max, icon_bg_color); // Background color
                            
                            // Compute centered icon position inside LayoutItemSize
                            ImVec2 icon_offset = ImVec2(
                                (LayoutItemSize.x - ThumbnailSize.x) * 0.5f,
                                (LayoutItemSize.y - ThumbnailSize.y) * 0.5f - 10
                            );

                            ImVec2 icon_min = ImVec2(
                                box_min.x + icon_offset.x,
                                box_min.y + icon_offset.y
                            );

                            ImVec2 icon_max = ImVec2(
                                icon_min.x + ThumbnailSize.x,
                                icon_min.y + ThumbnailSize.y
                            );

                            // Draw icon
                            draw_list->AddImage(
                                (void*)(intptr_t)EditorResources::Instance().GetIconAtlas()->GetTexture()->GetHandle(),
                                icon_min,
                                icon_max,
                                item_data->icon->uv0,
                                item_data->icon->uv1
                            );
                            /*if (ShowTypeOverlay && item_data->Type != 0)
                            {
                                ImU32 type_col = icon_type_overlay_colors[item_data->Type % IM_ARRAYSIZE(icon_type_overlay_colors)];
                                draw_list->AddRectFilled(ImVec2(box_max.x - 2 - icon_type_overlay_size.x, box_min.y + 2), ImVec2(box_max.x - 2, box_min.y + 2 + icon_type_overlay_size.y), type_col);
                            }*/
                            if (display_label)
                            {
                                ImU32 label_col = ImGui::GetColorU32(item_is_selected ? ImGuiCol_Text : ImGuiCol_TextDisabled);
                                //std::string filename = item_data->path.GetFilename();
                                std::string filename = item_data->path.GetFilename();
                                float width = ImGui::CalcTextSize(filename.c_str()).x;
                                draw_list->AddText(ImVec2(box_min.x + (box_max.x - box_min.x) * 0.5 - width / 2, box_max.y - ImGui::GetFontSize()), label_col, filename.c_str());
                            }
                        }

                        ImGui::PopID();
                    }
                }
            }
            clipper.End();
            ImGui::PopStyleVar(); // ImGuiStyleVar_ItemSpacing

            // Context menu
            if (ImGui::BeginPopupContextWindow())
            {
                ImGui::Text("Selection: %d items", selection.Size);
                ImGui::Separator();
                if (ImGui::MenuItem("Delete", "Del", false, selection.Size > 0))
                    requestDelete = true;
                ImGui::EndPopup();
            }

            ms_io = ImGui::EndMultiSelect();
            selection.ApplyRequests(ms_io);
            //if (want_delete)
                //selection.ApplyDeletionPostLoop(ms_io, items, item_curr_idx_to_focus);

            // Zooming with CTRL+Wheel
            if (ImGui::IsWindowAppearing())
                zoomWheelAccum = 0.0f;
            if (ImGui::IsWindowHovered() && io.MouseWheel != 0.0f && ImGui::IsKeyDown(ImGuiMod_Ctrl) && ImGui::IsAnyItemActive() == false)
            {
                zoomWheelAccum += io.MouseWheel;
                if (fabsf(zoomWheelAccum) >= 1.0f)
                {
                    // Calculate hovered item index from mouse location
                    // FIXME: Locking aiming on 'hovered_item_idx' (with a cool-down timer) would ensure zoom keeps on it.
                    const float hovered_item_nx = (io.MousePos.x - start_pos.x + LayoutItemSpacing * 0.5f) / LayoutItemStep.x;
                    const float hovered_item_ny = (io.MousePos.y - start_pos.y + LayoutItemSpacing * 0.5f) / LayoutItemStep.y;
                    const int hovered_item_idx = ((int)hovered_item_ny * LayoutColumnCount) + (int)hovered_item_nx;
                    //ImGui::SetTooltip("%f,%f -> item %d", hovered_item_nx, hovered_item_ny, hovered_item_idx); // Move those 4 lines in block above for easy debugging

                    // Zoom
                    thumbnailSize *= powf(1.1f, (float)(int)zoomWheelAccum);
                    thumbnailSize = IM_CLAMP(thumbnailSize, 16.0f, 128.0f);
                    zoomWheelAccum -= (int)zoomWheelAccum;
                    UpdateLayoutSizes(avail_width);

                    // Manipulate scroll to that we will land at the same Y location of currently hovered item.
                    // - Calculate next frame position of item under mouse
                    // - Set new scroll position to be used in next ImGui::BeginChild() call.
                    float hovered_item_rel_pos_y = ((float)(hovered_item_idx / LayoutColumnCount) + fmodf(hovered_item_ny, 1.0f)) * LayoutItemStep.y;
                    hovered_item_rel_pos_y += ImGui::GetStyle().WindowPadding.y;
                    float mouse_local_y = io.MousePos.y - ImGui::GetWindowPos().y;
                    ImGui::SetScrollY(hovered_item_rel_pos_y - mouse_local_y);
                }
            }
        }
        ImGui::EndChild();
    }
    
    void AssetBrowser::UpdateLayoutSizes(float avail_width)
    {
        LayoutItemSize = ItemSize;

        // Number of columns
        LayoutColumnCount = IM_MAX(
            (int)(avail_width / (LayoutItemSize.x + Spacing.x)),
            1
        );

        LayoutLineCount = (items.size() + LayoutColumnCount - 1) / LayoutColumnCount;

        LayoutItemStep = ImVec2(
            LayoutItemSize.x + Spacing.x,
            LayoutItemSize.y + Spacing.y
        );

        LayoutItemSpacing = Spacing.x;
        LayoutSelectableSpacing = IM_MAX(Spacing.x - IconHitSpacing, 0.0f);
        LayoutOuterPadding = Spacing.x * 0.5f;
    }

    void AssetBrowser::RequestOpenLevel(const Engine::Filesystem::Path &path)
    {
        auto& engine = Engine::Core::GetEngine();
        auto* levelManager = engine.GetLevelManager();

        for(int i = 0; i < levelManager->GetLoadedLevelCount(); i++){
            levelManager->UnloadLevel(i);
        }

        if (levelManager->IsAsyncLoadInProgress())
            return;

        // Whatever's currently loaded is only torn down once the new level has resolved
        // successfully (see LevelManager::FinishAsyncLoad) - so a bad/missing target here can't
        // leave the editor with zero levels loaded.
        std::string pathInProject = engine.GetFileManager()->GetFileInfos(path).nameInProject;
        levelManager->LoadLevelAsync(pathInProject);
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
            /// @todo modify assets database, resources, etc...
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