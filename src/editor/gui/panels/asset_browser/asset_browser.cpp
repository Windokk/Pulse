#include "asset_browser.hpp"
#include <QPushButton>

namespace Pulse::Editor::GUI{

    QPixmap AssetBrowser::IconForFile(const QString &path) const
    {
        QFileInfo info(path);
        QString ext = info.suffix().toLower();

        if (info.isDir())
            return SvgToPixmap(":/pulse/default/icons/folder.svg", QSize(64, 64));

        if (iconMap.contains(ext))
            return SvgToPixmap(iconMap[ext], QSize(64, 64));

        return SvgToPixmap(":/pulse/default/icons/unknown.svg", QSize(64, 64));
    }

    AssetBrowser::AssetBrowser(QWidget *parent) : QWidget(parent)
    {
        scrollArea = new QScrollArea(this);
        scrollArea->setWidgetResizable(true);

        contentWidget = new QWidget(scrollArea);
        contentWidget->setContentsMargins(20, 20, 20, 20);

        flowLayout = new QFlowLayout(contentWidget, 4, 30, 30);
        contentWidget->setLayout(flowLayout);
        scrollArea->setWidget(contentWidget);

        // ---- BREADCRUMB BAR ----
        breadcrumbWidget = new QWidget(this);
        breadcrumbLayout = new QHBoxLayout(breadcrumbWidget);
        breadcrumbLayout->setContentsMargins(10, 10, 10, 10);
        breadcrumbLayout->setSpacing(5);
        QFile styleSheetFile(":/pulse/default/stylesheets/default_asset_browser.qss");
	    if(styleSheetFile.open(QIODevice::ReadOnly)){
            QTextStream styleSheetStream(&styleSheetFile);
            QString result;
            result = styleSheetStream.readAll();
            styleSheetFile.close();
            breadcrumbWidget->setStyleSheet(result);
        }

        // MAIN LAYOUT
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->addWidget(breadcrumbWidget);   // Add ribbon bar
        mainLayout->addWidget(scrollArea);
        setLayout(mainLayout);

        NavigateTo(QString::fromStdString(Engine::Core::GetEngine().GetCurrentProject()->GetProjectResourcesPath().full));
    }

    void AssetBrowser::UpdateBreadcrumb()
    {
        // Clear previous buttons
        QLayoutItem* child;
        while ((child = breadcrumbLayout->takeAt(0)) != nullptr) {
            delete child->widget();
            delete child;
        }

        QString projectRoot = QString::fromStdString(
            Engine::Core::GetEngine().GetCurrentProject()->GetProjectResourcesPath().full
        );

        QString relative = QDir(projectRoot).relativeFilePath(
            QString::fromStdString(currentPath.full)
        );

        // If we're at the root, don't show any segments
        QStringList parts;
        if (relative != ".") {
            parts = relative.split("/", Qt::SkipEmptyParts);
        }

        QString accum = projectRoot;

        // Root button
        QPushButton* rootBtn = new QPushButton(
            QString::fromStdString(Engine::Core::GetEngine().GetCurrentProject()->GetProjectResourcesPath().GetFilename())
        );
        rootBtn->setFlat(true);
        connect(rootBtn, &QPushButton::clicked, this, [this, projectRoot]() {
            NavigateTo(projectRoot);
        });
        breadcrumbLayout->addWidget(rootBtn);

        // Add folders only if not in root
        for (const QString& part : parts) {
            breadcrumbLayout->addWidget(new QLabel("/"));

            accum += "/" + part;

            QPushButton* btn = new QPushButton(part);
            btn->setFlat(true);
            QString pathCopy = accum;

            connect(btn, &QPushButton::clicked, this, [this, pathCopy]() {
                NavigateTo(pathCopy);
            });

            breadcrumbLayout->addWidget(btn);
        }

        breadcrumbLayout->addStretch();
    }

    void AssetBrowser::NavigateTo(const QString& path){
        currentPath = path.toStdString();
        UpdateBreadcrumb();
        ReloadAssets();
    }

    void AssetBrowser::ReloadAssets()
    {
        while (flowLayout->count() > 0) {
            QLayoutItem *item = flowLayout->takeAt(0);
            if (item->widget())
                item->widget()->deleteLater();
            delete item;
        }

        for(Engine::Filesystem::FileInfos file_infos : Engine::Core::GetEngine().GetFileManager()->ListDirectory(currentPath,
            {Engine::Filesystem::Type::T_IMAGE,
            Engine::Filesystem::Type::T_SOUND,
            Engine::Filesystem::Type::T_FONT,
            Engine::Filesystem::Type::T_SHADER,
            Engine::Filesystem::Type::T_TEXT,
            Engine::Filesystem::Type::T_SCRIPT,
            Engine::Filesystem::Type::T_LEVEL,
            Engine::Filesystem::Type::T_MODEL,
            Engine::Filesystem::Type::T_MATERIAL,
            Engine::Filesystem::Type::T_CONFIG,
            Engine::Filesystem::Type::T_DIRECTORY},
            true, false)){

            AssetItem item = AssetItem({ QString::fromStdString(file_infos.path.GetFilename()), QString::fromStdString(file_infos.path.full), IconForFile(QString::fromStdString(file_infos.path.full)), file_infos.ID, file_infos.path.IsDirectory() });

            AssetItemWidget* widget = new AssetItemWidget(item, contentWidget);

            connect(widget, &AssetItemWidget::navigateRequested, this, &AssetBrowser::NavigateTo);
            connect(widget, &AssetItemWidget::renameRequested, this, &AssetBrowser::RenameAsset);

            flowLayout->addWidget(widget);
        }

        contentWidget->adjustSize();
    }

    void AssetBrowser::RenameAsset(const QString &oldPath, const QString &newName)
    {
        Engine::Filesystem::AssetIDManager* assetManager = Engine::Core::GetEngine().GetAssetIDManager();
        Engine::Filesystem::FileManager* fileManager = Engine::Core::GetEngine().GetFileManager();

        std::string nameInProject = fileManager->GetFileInfos(Engine::Filesystem::Path(oldPath.toStdString())).nameInProject;

        if(nameInProject == ""){
            DEBUG_ERROR("Tried renaming an asset outside of project !");
            return;
        }

        QFileInfo info(oldPath);
        QString newPath = info.dir().filePath(newName);

        if(QFile::exists(newPath)){
            DEBUG_ERROR("Cannot rename, file already exists:", newPath.toStdString());
            return;
        }

        if(!QFile::rename(oldPath, newPath)){
            DEBUG_ERROR("Failed to rename file :", oldPath.toStdString());
            return;
        }

        Engine::Core::GetEngine().GetAssetIDManager()->GetAssetFromID(assetManager->GetIDFromNameInProject(nameInProject))->baseInfos = fileManager->GetFileInfos(Engine::Filesystem::Path(newPath.toStdString()));
    }
}