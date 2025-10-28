#pragma once

#include <QWidget>
#include <QTreeView>
#include <QStandardItemModel>

#include "engine/levels/level.hpp"
#include "engine/ecs/objects/actors/actor.hpp"
#include "custom_tree.hpp"

#include <typeinfo>

namespace Epoch::Editor{

    class LevelTree : public QWidget{ 

        Q_OBJECT

        public:
            LevelTree(QWidget *parent = nullptr);
            void OnLevelStructureChanged(Engine::Events::LevelStructureChangedEvent event);

            void OnItemClicked(const QModelIndex &index, Qt::MouseButton button);

        private:
            CustomTreeView *treeView;
            QStandardItemModel *model;

            QStandardItem *GetItemFromObjectID(Engine::ECS::ObjectID objID);

            void SetupModel();

            /// @brief Clears the Level Tree then refills it with a level's hierarchy. Level has to be fully loaded
            /// @param level Shared pointer to the level object
            void LoadLevel(std::shared_ptr<Engine::Levels::Level> level);
            void populateTree(std::shared_ptr<Engine::ECS::Objects::Object> object, QStandardItem *parentItem);
            void SetupStyle();
    };


}