#include "level_tree.hpp"

#include <QVBoxLayout>
#include <QFile>
#include <QMenu>
#include <QThread>
#include <QApplication>
#include <QObject>

#include "engine/core/engine.hpp"

#include "editor/gui/main_win.hpp"

namespace Pulse::Editor::GUI{
    
    using Engine::Core::GetEngine;

    LevelTree::LevelTree() : treeView(new CustomTreeView(this)), model(new QStandardItemModel(this))
    {
        auto *layout = new QVBoxLayout(this);
        layout->addWidget(treeView);
        layout->setContentsMargins(0, 0, 0, 0);
        setLayout(layout);

        SetupModel();
        SetupStyle();

        GetEngine().GetEventDispatcher()->subscribeGlobal<Engine::Events::LevelStructureChangedEvent>([this](const Engine::Events::LevelStructureChangedEvent& event) {
            this->OnLevelStructureChanged(event);
        });

    }

    QStandardItem* LevelTree::GetItemFromObjectID(Engine::ECS::ObjectID objID){

        QList<QStandardItem*> items;

        for (int row = 0; row < model->rowCount(); ++row) {
            QStandardItem *topItem = model->item(row, 0);

            // Scan top level item
            if (topItem) {

                QVariant var = topItem->data(Qt::UserRole);
                Engine::ECS::ObjectID retrieved;

                if (var.isValid()) {
                    retrieved = var.value<Engine::ECS::ObjectID>();
                }

                if(retrieved.GetAsInt() == objID.GetAsInt()) {
                    items.append(topItem);
                    break; // skip scanning its children
                }
            }

            // Scan children
            for (int sub_row = 0; sub_row < topItem->rowCount(); ++sub_row) {
                QStandardItem *child = topItem->child(sub_row, 0);
                if (child) {

                    QVariant var = child->data(Qt::UserRole);
                    Engine::ECS::ObjectID retrieved;

                    if (var.isValid()) {
                        retrieved = var.value<Engine::ECS::ObjectID>();
                    }

                    if(retrieved.GetAsInt() == objID.GetAsInt()) {
                        items.append(child);
                        break; // skip scanning its children
                    }
                }
            }
        }
            

        if (items.isEmpty()){
            return nullptr;
        }

        QStandardItem *item = items.first();
        return item;
    }

    void LevelTree::OnLevelStructureChanged(Engine::Events::LevelStructureChangedEvent event){

        switch(event.changeType){
            case Engine::Events::CREATED:{

                std::shared_ptr<Pulse::Engine::ECS::Objects::Object> obj = GetEngine().GetObjectIDManager()->GetObjectFromID(event.sourceObjectID);

                QStandardItem *item = new QStandardItem(QString::fromStdString(event.actorName));
                item->setData(QVariant::fromValue(event.sourceObjectID), Qt::UserRole);

                if(obj->GetParent()){
                    QStandardItem* parentItem = GetItemFromObjectID(obj->GetParent()->GetID());
                    if(parentItem){
                        parentItem->appendRow(item);
                    }
                }
                else{
                    model->appendRow(item);
                }
                break;
            }
            case Engine::Events::DESTROYED:{
                if (!treeView)
                    return;

                QStandardItem* item = GetItemFromObjectID(event.sourceObjectID);
                if(!item)
                    return;

                QStandardItem *parentItem = item->parent();
                if (parentItem)
                    parentItem->removeRow(item->row());
                else
                    model->removeRow(item->row());

                break;
            }
            case Engine::Events::ACTIVATED:{
                break;
            }
            case Engine::Events::DEACTIVATED:{
                break;
            }
            case Engine::Events::MOVED:{
                break;
            }
            case Engine::Events::LOADED:{
                LoadLevel(GetEngine().GetResourcesManager()->GetLevel(GetEngine().GetAssetIDManager()->GetAssetFromID(Engine::Filesystem::AssetIDBuilder().WithValue(event.levelAssetID).Build())->baseInfos.nameInProject));
                model->setHorizontalHeaderLabels(QStringList() << QString::fromStdString(GetEngine().GetAssetIDManager()->GetAssetFromID(Engine::Filesystem::AssetIDBuilder().WithValue(event.levelAssetID).Build())->baseInfos.name));
                break;
            }
            default:
                break;
        }
    }

    void LevelTree::OnItemClicked(const QModelIndex &index, Qt::MouseButton button){
        
        if (!index.isValid()) return;

        QStandardItem *item = model->itemFromIndex(index);
        if (item) {
            QVariant var = item->data(Qt::UserRole);
            Engine::ECS::ObjectID retrieved;

            if (var.isValid()) {
                retrieved = var.value<Engine::ECS::ObjectID>();
            }

            std::shared_ptr<Engine::ECS::Objects::Object> obj = GetEngine().GetObjectIDManager()->GetObjectFromID(retrieved);
            std::shared_ptr<Engine::ECS::Objects::Actor> actor = std::dynamic_pointer_cast<Engine::ECS::Objects::Actor>(obj);
            
            if (button == Qt::RightButton) {
                QMenu menu(treeView);

                menu.setStyleSheet(R"(
                        QMenu {
                            border: 2px solid #444;
                            border-radius: 6px;
                            background-color: #2c2c2c;
                            color: white;
                        }
                        QMenu::item {
                            height: 24px;
                            padding-left : 10px;
                            border-radius: 4px;
                        }
                        QMenu::item:selected {
                            background-color: #444;
                        }
                        QMenu::icon {
                            height: 24px;
                            padding-left : 10px;
                        }
                    )");

                QAction *renameAction = menu.addAction(QIcon(":/pulse/default/icons/rename.svg"), "Rename");
                QAction *deleteAction = menu.addAction(QIcon(":/pulse/default/icons/delete.svg"), "Delete");
                QAction *duplicateAction = menu.addAction(QIcon(":/pulse/default/icons/duplicate.svg"), "Duplicate");
                menu.addSeparator();
                QAction *createAction = menu.addAction(QIcon(":/pulse/default/icons/add.svg"), "Create Actor");

                // Show the menu at the current cursor position
                QAction *selectedAction = menu.exec(QCursor::pos());

                if (selectedAction == renameAction) {
                    treeView->edit(index);
                }
                else if (selectedAction == deleteAction) {
                    actor->Destroy();
                }
                else if (selectedAction == duplicateAction) {
                    actor->Clone();
                }
                else if (selectedAction == createAction) {
                    std::shared_ptr<Engine::ECS::Objects::Actor> child = Engine::ECS::Objects::Object::Create<Engine::ECS::Objects::Actor>("New Actor");
                    actor->AddChild(child);
                }
            }
        
            else if(button == Qt::LeftButton){
                parent->SetSelectedActor(actor);
            }
        }
    }

    void LevelTree::ClickOutsideItems(Qt::MouseButton button){
        if (button == Qt::LeftButton){
            parent->SetSelectedActor(nullptr);
            treeView->clearSelection();
        }
    }

    void LevelTree::SetParentWindow(EditorMainWindow *parent)
    {
        this->parent = parent;
    }

    void LevelTree::SetupModel()
    {
        treeView->setModel(model);
        treeView->setUniformRowHeights(true);
        treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
        treeView->setAlternatingRowColors(false);
        
        connect(treeView, &CustomTreeView::itemClicked, this, [this](const QModelIndex &index, Qt::MouseButton button) { OnItemClicked(index, button); });
    
        connect(treeView, &CustomTreeView::noItemClicked, this, [this](Qt::MouseButton button) { ClickOutsideItems(button); });

        connect(model, &QStandardItemModel::itemChanged, this, [this](QStandardItem *changedItem) {
            QVariant var = changedItem->data(Qt::UserRole);
            if (!var.isValid()) return;

            Engine::ECS::ObjectID id = var.value<Engine::ECS::ObjectID>();
            auto obj = GetEngine().GetObjectIDManager()->GetObjectFromID(id);
            auto actor = std::dynamic_pointer_cast<Engine::ECS::Objects::Actor>(obj);
            if (actor) {
                QString newName = changedItem->text();
                actor->SetName(newName.toStdString());
            }
        });
    
    }

    void LevelTree::LoadLevel(std::shared_ptr<Engine::Levels::Level> level)
    {
        model->clear();

        for (const auto& rootActor : level->GetRootActors()) {
            if (!rootActor)
                continue;

            QStandardItem *rootItem = new QStandardItem(QString::fromStdString(rootActor->GetName()));
            rootItem->setData(QVariant::fromValue(rootActor->GetID()), Qt::UserRole);
            model->appendRow(rootItem);

            // Recursively populate children
            populateTree(rootActor, rootItem);
    
        }
    }

    void LevelTree::populateTree(std::shared_ptr<Engine::ECS::Objects::Object> object, QStandardItem *parentItem)
    {
        for (Engine::ECS::ObjectID childID : object->GetChildrenID(false)) {
            std::shared_ptr<Engine::ECS::Objects::Object> obj = object->GetChild(childID);
            std::shared_ptr<Engine::ECS::Objects::Actor> child = std::dynamic_pointer_cast<Engine::ECS::Objects::Actor>(obj);
            if (!child)
                continue;

            QStandardItem *item = new QStandardItem(QString::fromStdString(child->GetName()));
            item->setData(QVariant::fromValue(child->GetID()), Qt::UserRole);
            parentItem->appendRow(item);

            // Recurse into children
            populateTree(child, item);
        }
    }

    void LevelTree::SetupStyle() {
        
        QFile styleSheetFile(":/pulse/default/stylesheets/default_tree.qss");
	    if(styleSheetFile.open(QIODevice::ReadOnly)){
            QTextStream styleSheetStream(&styleSheetFile);
            QString result;
            result = styleSheetStream.readAll();
            styleSheetFile.close();
            treeView->setStyleSheet(result);
        }
    }
}

