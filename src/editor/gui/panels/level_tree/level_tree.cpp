#include "level_tree.hpp"

#include "engine/core/engine.hpp"

#include "editor/gui/main_window.hpp"

#include "engine/levels/level_manager.hpp"

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

    void LevelTree::SetSelection(std::shared_ptr<Engine::Objects::Actor> actor)
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

    void LevelTree::DrawActorNode(std::shared_ptr<Engine::Objects::Actor> actor)
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

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            selectedID = actor->GetID();
            if (parent)
                parent->SetSelectedActor(actor);
        }

        // Context Menu
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Rename"))
            {
                renamingActor = actor;
                strcpy(renameBuffer, actor->GetName().c_str());
                openRenamingPopup = true;
            }

            if (ImGui::MenuItem("Delete"))
            {
                if (actor == parent->GetSelectedActor())
                    parent->SetSelectedActor(nullptr);

                actor->Destroy();
            }

            if (ImGui::MenuItem("Duplicate"))
            {
                actor->Clone();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Create Actor"))
            {
                auto child = Engine::Core::Object::CreateWithContext<Engine::Objects::Actor>(&Engine::Core::GetEngine(), "New Actor", &Engine::Core::GetEngine());
                actor->AddChild(child);
            }

            ImGui::EndPopup();
        }

        if(openRenamingPopup){
            ImGui::OpenPopup("RenameActorPopup");
            openRenamingPopup = false;
        }

        // Rename Popup
        if (ImGui::BeginPopup("RenameActorPopup"))
        {
            ImGui::InputText("##ActorName", renameBuffer, sizeof(renameBuffer));

            if (ImGui::Button("OK") || Engine::Core::GetEngine().GetInputManager()->WasKeyReleased(Engine::Input::Key::Enter))
            {
                if (renamingActor)
                    renamingActor->SetName(renameBuffer);

                renamingActor = nullptr;
                openRenamingPopup = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel"))
            {
                renamingActor = nullptr;
                openRenamingPopup = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        if (open)
        {
            for (auto childID : actor->GetChildrenID(false))
            {
                auto obj = actor->GetChild(childID);
                auto child = std::dynamic_pointer_cast<Engine::Objects::Actor>(obj);
                if (child)
                    DrawActorNode(child);
            }

            ImGui::TreePop();
        }
    }
}

