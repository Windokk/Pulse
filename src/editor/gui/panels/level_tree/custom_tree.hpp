#pragma once

#include <QTreeView>
#include <QMouseEvent>

namespace Pulse::Editor{

    class CustomTreeView : public QTreeView {
        Q_OBJECT
    public:
        using QTreeView::QTreeView;

    signals:
        void itemClicked(const QModelIndex &index, Qt::MouseButton button);
        void noItemClicked(Qt::MouseButton button);

    protected:
        void mousePressEvent(QMouseEvent *event) override;
    };
}