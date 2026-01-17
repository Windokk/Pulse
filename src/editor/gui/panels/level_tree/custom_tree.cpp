#include "custom_tree.hpp"

namespace Pulse::Editor::GUI {
    
    void CustomTreeView::mousePressEvent(QMouseEvent *event)
    {
        QModelIndex index = indexAt(event->pos());
        if (!index.isValid()) {
            emit noItemClicked(event->button());
            QTreeView::mousePressEvent(event);
            return;
        }

        bool triggerClick = false;

        if (model()->hasChildren(index)) {
            QStyleOptionViewItem option;
            option.initFrom(this);
            option.rect = visualRect(index);
            option.state |= QStyle::State_Children;
            if (isExpanded(index))
                option.state |= QStyle::State_Open;

            QRect branchRect = style()->subElementRect(
                QStyle::SE_TreeViewDisclosureItem,
                &option,
                this
            );

            if (branchRect.contains(event->pos())) {
                triggerClick = true;
            }
        } else {
            triggerClick = true;
        }

        if (triggerClick) {
            emit itemClicked(index, event->button());
        }

        QTreeView::mousePressEvent(event);
    }
}