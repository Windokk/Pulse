#pragma once

#include <QWidget>
#include <QTreeView>
#include <QStandardItemModel>

#include "engine/levels/level.hpp"
#include "engine/ecs/objects/actors/actor.hpp"
#include "custom_tree.hpp"

#include <typeinfo>

namespace Pulse::Editor::GUI{

    class EditorMainWindow;

    class LevelTree : public QWidget{ 

        public:
            LevelTree();
            void OnLevelStructureChanged(Engine::Events::LevelStructureChangedEvent event);

            void OnItemClicked(const QModelIndex &index, Qt::MouseButton button);

            void ClickOutsideItems(Qt::MouseButton button);

            void SetParentWindow(EditorMainWindow *parent);

            void ClearSelection(){
                if(treeView){
                    treeView->clearSelection();
                }
            }

            void SetSelection(std::shared_ptr<Engine::ECS::Objects::Actor> newSelection){
                if(treeView){
                    QModelIndex item = GetItemFromObjectID(newSelection->GetID())->index();
                    treeView->selectionModel()->select(item, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                }
            }

        private:
            CustomTreeView *treeView;
            QStandardItemModel *model;

            QStandardItem *GetItemFromObjectID(Engine::Core::ObjectID objID);

            void SetupModel();

            /// @brief Clears the Level Tree then refills it with a level's hierarchy. Level has to be fully loaded
            /// @param level Shared pointer to the level object
            void LoadLevel(std::shared_ptr<Engine::Levels::Level> level);
            void populateTree(std::shared_ptr<Engine::ECS::Objects::LevelObject> object, QStandardItem *parentItem);
            void SetupStyle();

            EditorMainWindow* parent = nullptr;
    };


}