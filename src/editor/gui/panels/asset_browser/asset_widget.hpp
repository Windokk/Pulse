#pragma once
#include <QIcon>
#include <QSvgRenderer>
#include <QPainter>
#include <QMouseEvent>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

#include "engine/filesystem/assetID.hpp"


namespace Pulse::Editor::GUI{

    struct AssetItem {
        QString name;
        QString path;
        QPixmap icon;
        Engine::Filesystem::AssetID ID;
        bool dir;
    };

    static QPixmap SvgToPixmap(const QString &path, const QSize &target)
    {
        QSvgRenderer svg(path);

        QPixmap pixmap(target);
        pixmap.fill(Qt::transparent);

        QPainter p(&pixmap);
        p.setRenderHint(QPainter::Antialiasing);

        QSize svgSize = svg.defaultSize().scaled(target, Qt::KeepAspectRatio);
        QRect r(QPoint(0,0), svgSize);
        r.moveCenter(QRect(QPoint(0,0), target).center());

        svg.render(&p, r);
        return pixmap;
    }

    static QMap<QString, QString> iconMap = {
        {"png", ":/pulse/default/icons/texture.svg"},
        {"jpg", ":/pulse/default/icons/texture.svg"},
        {"jpeg", ":/pulse/default/icons/texture.svg"},
        {"hdr", ":/pulse/default/icons/texture.svg"},
        {"tga", ":/pulse/default/icons/texture.svg"},
        {"bmp", ":/pulse/default/icons/texture.svg"},
        {"jpg", ":/pulse/default/icons/texture.svg"},

        {"fbx", ":/pulse/default/icons/model.svg"},

        {"wav", ":/pulse/default/icons/audio.svg"},
        {"mp3", ":/pulse/default/icons/audio.svg"},
        {"ogg", ":/pulse/default/icons/audio.svg"},

        {"mat", ":/pulse/default/icons/material.svg"},
        {"material", ":/pulse/default/icons/material.svg"},

        {"level", ":/pulse/default/icons/level.svg"},
        {"lvl", ":/pulse/default/icons/level.svg"},

        {"ttf", ":/pulse/default/icons/font.svg"},
        {"otf", ":/pulse/default/icons/font.svg"},

        {"geom", ":/pulse/default/icons/shader.svg"},
        {"vert", ":/pulse/default/icons/shader.svg"},
        {"frag", ":/pulse/default/icons/shader.svg"},

        {"txt",  ":/pulse/default/icons/text.svg"},
        {"md",  ":/pulse/default/icons/text.svg"},

        {"cpp", ":/pulse/default/icons/code.svg"},
        {"hpp", ":/pulse/default/icons/code.svg"},

        {"json",  ":/pulse/default/icons/config.svg"},
        {"ini",  ":/pulse/default/icons/config.svg"},
        {"cfg",  ":/pulse/default/icons/config.svg"}
    };

    class AssetItemWidget : public QWidget {
        Q_OBJECT
        public:
            AssetItemWidget(const AssetItem& asset, QWidget* parent = nullptr);

        signals:
            void navigateRequested(const QString& path);
            void renameRequested(const QString& oldPath, const QString& newName);

        protected:
            void mouseDoubleClickEvent(QMouseEvent* event) override;

            void enterEvent(QEnterEvent *event) override {
                hovered = true;
                update();
                QWidget::enterEvent(event);
            }

            void leaveEvent(QEvent* event) override {
                hovered = false;
                update();
                QWidget::leaveEvent(event);
            }

            void paintEvent(QPaintEvent* event) override;

        private slots:
            void startEditing();

            void finishEditing();

        private:
            AssetItem asset;
            QLabel* iconLabel;
            QLabel* nameLabel;
            QLineEdit* nameEdit;
            bool hovered = false;
    };
}