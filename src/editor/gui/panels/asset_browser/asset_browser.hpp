#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QListView>
#include <QFileSystemModel>
#include <QScrollArea>

#include "flow_layout.hpp"

#include "asset_widget.hpp"

#include "engine/core/engine.hpp"

namespace Pulse::Editor::GUI{

    class AssetBrowser : public QWidget{
        Q_OBJECT

        public:
            QPixmap IconForFile(const QString &path) const;
            AssetBrowser(QWidget *parent = nullptr);

            void UpdateBreadcrumb();

            void ReloadAssets();

        public slots:
            void NavigateTo(const QString& path);
            void RenameAsset(const QString& oldPath, const QString& newName);

        private:

            QScrollArea* scrollArea;
            QWidget* contentWidget;
            QFlowLayout* flowLayout;

            QWidget* breadcrumbWidget;
            QHBoxLayout* breadcrumbLayout;

            Engine::Filesystem::Path currentPath;

    };
}