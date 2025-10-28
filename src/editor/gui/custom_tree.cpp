#include "custom_tree.hpp"

namespace Epoch::Editor {
    void CustomTreeView::mousePressEvent(QMouseEvent *event)
    {
        QModelIndex index = indexAt(event->pos());
        if (index.isValid()) {
            emit itemClicked(index, event->button());
        }
        QTreeView::mousePressEvent(event); // call base implementation
    }
}