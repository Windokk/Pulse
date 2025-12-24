#include "asset_widget.hpp"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QGraphicsDropShadowEffect>

namespace Pulse::Editor::GUI{

    AssetItemWidget::AssetItemWidget(const AssetItem& asset, QWidget* parent) : QWidget(parent), asset(asset)
    {
        iconLabel = new QLabel(this);
        iconLabel->setPixmap(asset.icon);
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setFixedSize(64, 64);
        
        int margin = 8;

        int sideMargins = margin * 2;
        int w = iconLabel->width() + sideMargins;

        nameLabel = new QLabel(asset.name, this);
        nameLabel->setAlignment(Qt::AlignCenter);
        nameLabel->setWordWrap(true);
        nameLabel->setFixedWidth(iconLabel->width());

        QFontMetrics fm(nameLabel->font());
        QString elided = fm.elidedText(asset.name, Qt::ElideRight, nameLabel->width());
        nameLabel->setText(elided);

        nameEdit = new QLineEdit(this);
        nameEdit->setText(asset.name);
        nameEdit->setAlignment(Qt::AlignCenter);
        nameEdit->hide();
        nameEdit->setFrame(false);

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setSpacing(2);
        layout->setContentsMargins(margin,margin,margin,margin);
        layout->addWidget(iconLabel, 0, Qt::AlignHCenter);
        layout->addWidget(nameLabel, 0, Qt::AlignHCenter);
        layout->addWidget(nameEdit, 0, Qt::AlignHCenter);

        setLayout(layout);

        // Finish editing on Enter or focus out
        connect(nameEdit, &QLineEdit::editingFinished, this, &AssetItemWidget::finishEditing);

        setMouseTracking(true);

        auto *shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(12);
        shadow->setOffset(3, 3);
        shadow->setColor(QColor(0, 0, 0, 200));  // soft shadow
        setGraphicsEffect(shadow);
    }

    void AssetItemWidget::mouseDoubleClickEvent(QMouseEvent *event)
    {
        // Double-click on icon
        if (iconLabel->geometry().contains(event->pos())) {
            if(asset.dir)
                emit navigateRequested(asset.path);
            return;
        }

        // Double-click on name
        if (nameLabel->geometry().contains(event->pos())) {
            startEditing();
            return;
        }

        QWidget::mouseDoubleClickEvent(event);
    }

    void AssetItemWidget::startEditing()
    {
        nameLabel->hide();
        nameEdit->show();
        nameEdit->setFocus();
        nameEdit->selectAll();
    }

    void AssetItemWidget::paintEvent(QPaintEvent *event)
    {
        QWidget::paintEvent(event);

        if (!hovered) return;

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, false);

        QRect box = rect().adjusted(1, 1, -1, -1);

        QColor hoverColor = QColor(90, 90, 90, 40);
        painter.fillRect(box, hoverColor);

        painter.setPen(QPen(QColor(140, 140, 140, 120), 1));
        painter.drawRect(box);
    }
    
    void AssetItemWidget::finishEditing()
    {
        QString newName = nameEdit->text().trimmed();
        if(!newName.isEmpty() && newName != asset.name){
            emit renameRequested(asset.path, newName);
            asset.name = newName;
            nameLabel->setText(newName);
        }
        nameEdit->hide();
        nameLabel->show();
    }


    
}