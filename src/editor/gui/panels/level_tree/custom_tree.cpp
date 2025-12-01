#include "custom_tree.hpp"

namespace Pulse::Editor {
    void CustomTreeView::mousePressEvent(QMouseEvent *event)
    {
        QModelIndex index = indexAt(event->pos());
        if (index.isValid()) {
            emit itemClicked(index, event->button());
        }
        else{
            emit noItemClicked(event->button());
        }
        QTreeView::mousePressEvent(event); // call base implementation
    }
}