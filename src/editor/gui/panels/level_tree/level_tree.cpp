#include "level_tree.hpp"

#include "engine/core/engine.hpp"

#include "editor/gui/main_window.hpp"

namespace Pulse::Editor::GUI{
    
    using Engine::Core::GetEngine;

    void LevelTree::Draw()
    {
        if (!ImGui::Begin("Level"))
        {
            ImGui::End();
            return;
        }

        auto level = GetEngine().GetLevelManager()->GetLevelAt(0);
        if (!level)
        {
            ImGui::Text("No level loaded");
            ImGui::End();
            return;
        }

        for (const auto& [id, rootActor] : level->GetRootActors())
        {
            if (rootActor)
                DrawActorNode(rootActor);
        }

        // Click vide = deselect
        if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
        {
            if (!ImGui::IsAnyItemHovered())
            {
                selectedID = {};
                if (parent)
                    parent->SetSelectedActor(nullptr);
            }
        }

        ImGui::End();
    }
    

    void LevelTree::SetParentWindow(Core::EditorMainWindow *parent)
    {
        this->parent = parent;
    }

    void LevelTree::SetSelection(std::shared_ptr<Engine::ECS::Objects::Actor> actor)
    {
        if (actor)
        {
            selectedID = actor->GetID();
        }
        else
        {
            selectedID = {};
        }
    }

    void LevelTree::DrawActorNode(std::shared_ptr<Engine::ECS::Objects::Actor> actor)
    {
        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanFullWidth;

        if (actor->GetChildrenID(false).empty())
            flags |= ImGuiTreeNodeFlags_Leaf;

        if (selectedID == actor->GetID())
            flags |= ImGuiTreeNodeFlags_Selected;

        bool open = ImGui::TreeNodeEx(
            (void*)(intptr_t)actor->GetID().GetAsInt(),
            flags,
            actor->GetName().c_str()
        );

        // Left click selection
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            selectedID = actor->GetID();
            if (parent)
                parent->SetSelectedActor(actor);
        }

        // Right click menu
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Rename"))
            {
                
            }

            if (ImGui::MenuItem("Delete"))
            {
                actor->Destroy();
            }

            if (ImGui::MenuItem("Duplicate"))
            {
                actor->Clone();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Create Actor"))
            {
                auto child = Engine::Core::Object::Create<Engine::ECS::Objects::Actor>("New Actor");
                actor->AddChild(child);
            }

            ImGui::EndPopup();
        }

        if (open)
        {
            for (auto childID : actor->GetChildrenID(false))
            {
                auto obj = actor->GetChild(childID);
                auto child = std::dynamic_pointer_cast<Engine::ECS::Objects::Actor>(obj);
                if (child)
                    DrawActorNode(child);
            }

            ImGui::TreePop();
        }
    }
}

