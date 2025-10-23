#include "level_tree.hpp"

#include <QVBoxLayout>

#include "engine/core/engine.hpp"

namespace Epoch::Editor{
    
    LevelTree::LevelTree(QWidget *parent): QWidget(parent), treeView(new QTreeView(this)), model(new QStandardItemModel(this))
    {
        auto *layout = new QVBoxLayout(this);
        layout->addWidget(treeView);
        layout->setContentsMargins(0, 0, 0, 0);
        setLayout(layout);

        SetupModel();
        SetupStyle();

        Engine::Core::GetEngine().GetEventDispatcher()->subscribeGlobal<Engine::Events::LevelStructureChangedEvent>([this](const Engine::Events::LevelStructureChangedEvent& event) {
            this->OnLevelStructureChanged(event);
        });

    }

    void LevelTree::OnLevelStructureChanged(Engine::Events::LevelStructureChangedEvent event){

        switch(event.changeType){
            case Engine::Events::CREATED:{

            }
            case Engine::Events::DESTROYED:{

            }
            case Engine::Events::ACTIVATED:{

            }
            case Engine::Events::DEACTIVATED:{

            }
            case Engine::Events::MOVED:{

            }
        }

        std::string levelName = Engine::Core::GetEngine().GetAssetIDManager()->GetAssetFromID(Engine::Filesystem::AssetID(event.levelID))->baseInfos.nameInProject;

        LoadLevel(*Engine::Core::GetEngine().GetResourcesManager()->GetLevel(levelName).get());

    }

    void LevelTree::SetupModel()
    {
        QStandardItem *rootItem = model->invisibleRootItem();

        QStandardItem *parentItem = new QStandardItem("Parent Node");
        parentItem->appendRow(new QStandardItem("Child Node 1"));
        parentItem->appendRow(new QStandardItem("Child Node 2"));

        rootItem->appendRow(parentItem);
        model->setHorizontalHeaderLabels(QStringList() << "Scene");

        treeView->setModel(model);
        treeView->setHeaderHidden(true);
        treeView->setUniformRowHeights(true);
        treeView->setAlternatingRowColors(true);
    }

    void LevelTree::LoadLevel(Engine::Levels::Level level)
    {
        model->clear();
        model->setHorizontalHeaderLabels({ "Name" }); // or ID, Type, etc.

        for (const auto& rootActor : level.GetRootActors()) {
            if (!rootActor)
                continue;

            QStandardItem *rootItem = new QStandardItem(QString::fromStdString(rootActor->GetName()));
            model->appendRow(rootItem);

            // Recursively populate children
            populateTree(rootActor, rootItem);
    
        }

        treeView->expandAll(); // Optional  
    }

    void LevelTree::populateTree(std::shared_ptr<Engine::ECS::Objects::Object> object, QStandardItem *parentItem)
    {
        for (Engine::ECS::ObjectID childID : object->GetChildrenID(false)) {
            std::shared_ptr<Engine::ECS::Objects::Object> obj = object->GetChild(childID);
            std::shared_ptr<Engine::ECS::Objects::Actor> child = std::dynamic_pointer_cast<Engine::ECS::Objects::Actor>(obj);
            if (!child)
                continue;

            QStandardItem *item = new QStandardItem(QString::fromStdString(child->GetName()));
            parentItem->appendRow(item);

            // Recurse into children
            populateTree(child, item);
        }
    }

    void LevelTree::SetupStyle() {
        QString style = R"(
            QTreeView {
                background-color: #1e1e1e;
                color: #dcdcdc;
                alternate-background-color: #2b2b2b;
                show-decoration-selected: 1;
                selection-background-color: #3d8fd1;
                selection-color: #ffffff;
                border: none;
                font-size: 13px;
            }

            QTreeView::item {
                height: 24px;
                padding: 4px;
            }

            QTreeView::item:selected {
                background-color: #3d8fd1;
                color: white;
            }

            QTreeView::branch:has-children:!has-siblings:closed,
            QTreeView::branch:closed:has-children:has-siblings {
                image: url(:/icons/arrow-right.svg);
            }

            QTreeView::branch:open:has-children:!has-siblings,
            QTreeView::branch:open:has-children:has-siblings {
                image: url(:/icons/arrow-down.svg);
            }
        )";

        treeView->setStyleSheet(style);
    }
}

