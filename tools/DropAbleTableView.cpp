#include "DropAbleTableView.h"
#include <QDropEvent>
#include <QKeyEvent>
#include <QAbstractItemModel>
#include <QMessageBox>
#include <QMimeData>
#include <exception>

DropAbleTableView::DropAbleTableView(QWidget *parent) : QTableView(parent)
{
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    //setSelectionBehavior(QAbstractItemView::SelectRows);
    setDragEnabled(true);
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DragDropMode::DragDrop);
    setDefaultDropAction(Qt::DropAction::MoveAction);
    setDragDropOverwriteMode(false);
    setDropIndicatorShown(true);
    setSortingEnabled(true);
}

void DropAbleTableView::tryDeleteSelectedRows()
{
    if (!selectionModel()) {
        return;
    }
    auto rows = selectionModel()->selectedRows();
    if (!rows.empty()) {
        auto button = QMessageBox::question(this, "Delete", "Delete selected row(s)?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No);
        if (button == QMessageBox::StandardButton::Yes) {
            emit deleteRows(rows);
        }
    }
}

void DropAbleTableView::dropEvent(QDropEvent *event)
{
    QTableView::dropEvent(event);
}

void DropAbleTableView::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key::Key_Delete:
    case Qt::Key::Key_Backspace:
        tryDeleteSelectedRows();
        return;
    default:
        QTableView::keyPressEvent(event);
        break;
    }
}
