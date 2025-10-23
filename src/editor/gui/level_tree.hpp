#pragma once

#include <QWidget>
#include <QTreeView>
#include <QStandardItemModel>

#include "engine/levels/level.hpp"
#include "engine/ecs/objects/actors/actor.hpp"

namespace Epoch::Editor{

    class LevelTree : public QWidget{

        Q_OBJECT

        public:
            LevelTree(QWidget *parent = nullptr);

            void OnLevelStructureChanged(Engine::Events::LevelStructureChangedEvent event);

        private:
            QTreeView *treeView;
            QStandardItemModel *model;

            void SetupModel();
            void LoadLevel(Engine::Levels::Level level);
            void populateTree(std::shared_ptr<Engine::ECS::Objects::Object> object, QStandardItem *parentItem);
            void SetupStyle();
    };


}