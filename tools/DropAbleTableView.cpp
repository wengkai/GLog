#include "DropAbleTableView.h"
#include "MultiLineDelegate.h"
#include "GHeaderView.h"
#include <QDropEvent>
#include <QKeyEvent>
#include <QAbstractItemModel>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <QMimeData>
#include <QMenu>
#include <QThread>
#include <exception>

DropAbleTableView::DropAbleTableView(QWidget *parent) : QTableView(parent)
{
    setItemDelegate(new MultiLineDelegate(this));
    headerview = new GHeaderView(Qt::Orientation::Horizontal, this);
    setHorizontalHeader(headerview);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setDragEnabled(true);
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DragDropMode::DragDrop);
    setDefaultDropAction(Qt::DropAction::MoveAction);
    setDragDropOverwriteMode(false);
    setDropIndicatorShown(true);
    setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
    connect(this, &DropAbleTableView::customContextMenuRequested, this, &DropAbleTableView::customContextMenu);
    connect(this, &DropAbleTableView::setMSelectionSignal, this, &DropAbleTableView::setMSelection);
}

void DropAbleTableView::tryDeleteSelectedRows()
{
    auto rows = selectionModel()->selectedRows();
    if (!rows.empty()) {
        auto button = QMessageBox::question(this, tr("Delete"), tr("Delete selected row(s)?"), QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No);
        if (button == QMessageBox::StandardButton::Yes) {
            emit deleteRowsSignal(rows);
        }
    }
}

void DropAbleTableView::tryNewSelectedRowsView()
{
    auto rows = selectionModel()->selectedRows();
    if (!rows.empty()) {
        auto button = QMessageBox::question(this, tr("New view"), tr("Create new view with selected row(s)?"), QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No);
        if (button == QMessageBox::StandardButton::Yes) {
            emit newViewWithRowsSignal(rows);
        }
    }
}

void DropAbleTableView::tryPasteRows()
{
    auto clipboard = QGuiApplication::clipboard();
    auto mimeData = clipboard->mimeData();
    if (mimeData) {
        auto button = QMessageBox::question(this, tr("Paste"), tr("Paste rows from clipboard?"), QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No);
        if (button == QMessageBox::StandardButton::Yes) {
            emit pasteRowsSignal(mimeData);
        }
    }
}

void DropAbleTableView::tryCopySelectedRows()
{
    auto rows = selectionModel()->selectedRows();
    if (!rows.empty()) {
        emit copyRowsSignal(rows);
    }
}

GHeaderView *DropAbleTableView::getHeaderView()
{
    return headerview;
}

void DropAbleTableView::findNext(QString key, QString value, bool isReg)
{
    if (!model()->rowCount()) {
        emit selected();
        return;
    }
    auto current = selectionModel()->currentIndex();
    if (!current.isValid()) {
        emit findNextSignal(current, key, value, isReg);
        return;
    }
    int row = current.row();
    int column = current.column();
    if (key.isEmpty()) {
        if (column + 1 < model()->columnCount()) {
            current = model()->index(row, column + 1);
        } else if (row + 1 < model()->rowCount()) {
            current = model()->index(row + 1, 0);
        } else {
            current = QModelIndex();
        }
    } else {
        if (row + 1 < model()->rowCount()) {
            current = model()->index(row + 1, column);
        } else {
            current = QModelIndex();
        }
    }
    
    emit findNextSignal(current, key, value, isReg);
}

void DropAbleTableView::foundNext(QModelIndex index)
{
    if (index.isValid()) {
        selectionModel()->setCurrentIndex(index, QItemSelectionModel::SelectionFlag::ClearAndSelect);
        scrollTo(index, QAbstractItemView::ScrollHint::PositionAtCenter);
    } else {
        QMessageBox::information(this, tr("Find"), tr("No more matching record found."));
    }
    emit selected();
}

void DropAbleTableView::selectRows(QList<int> rows)
{
    m_selection = selectionModel()->selection();
    auto m_model = model();
    auto columnCount = m_model->columnCount();
    // QItemSelection::merge can be time consuming when there are many rows to be selected
    auto mergeSub = QThread::create([=](){
        for (auto& row : rows) {
            auto index = m_model->index(row, 0);
            auto index2 = m_model->index(row, columnCount - 1);
            m_selection.merge(QItemSelection(index, index2), QItemSelectionModel::SelectionFlag::Select);
        }
        emit setMSelectionSignal(QItemSelectionModel::SelectionFlag::ClearAndSelect);
    });
    connect(mergeSub, &QThread::finished, mergeSub, &QThread::deleteLater);
    mergeSub->start();
}

void DropAbleTableView::deselectRows(QList<int> rows)
{
    m_selection = selectionModel()->selection();
    auto m_model = model();
    auto columnCount = m_model->columnCount();
    // QItemSelection::merge can be time consuming when there are many rows to be selected
    auto mergeSub = QThread::create([=](){
        for (auto& row : rows) {
            auto index = m_model->index(row, 0);
            auto index2 = m_model->index(row, columnCount - 1);
            m_selection.merge(QItemSelection(index, index2), QItemSelectionModel::SelectionFlag::Deselect);
        }
        emit setMSelectionSignal(QItemSelectionModel::SelectionFlag::ClearAndSelect);
    });
    connect(mergeSub, &QThread::finished, mergeSub, &QThread::deleteLater);
    mergeSub->start();
}

void DropAbleTableView::setMSelection(QItemSelectionModel::SelectionFlag command)
{
    selectionModel()->select(m_selection, command);
    emit selected();
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

void DropAbleTableView::customContextMenu(const QPoint &pos) {
    auto index = indexAt(pos);
    auto contextMenu = new QMenu(this);
    if (index.isValid()) {
        if (!selectionModel()->selectedRows().empty()){
            QAction *actionDelete = contextMenu->addAction(tr("delete row(s)"));
            connect(actionDelete, &QAction::triggered, this, [=]() {
                tryDeleteSelectedRows();
            });
            QAction *actionNewView = contextMenu->addAction(tr("new view with row(s)"));
            connect(actionNewView, &QAction::triggered, this, [=]() {
                tryNewSelectedRowsView();
            });
            QAction *actionCopy = contextMenu->addAction(tr("copy row(s)"));
            connect(actionCopy, &QAction::triggered, this, [=]() {
                tryCopySelectedRows();
            });
        }
        QAction *actionCopyCell = contextMenu->addAction(tr("copy cell content"));
        connect(actionCopyCell, &QAction::triggered, this, [=]() {      
            auto data = model()->data(index).toString();
            QApplication::clipboard()->setText(data);
        });
    }
    QAction *actionPaste = contextMenu->addAction(tr("paste"));
    connect(actionPaste, &QAction::triggered, this, [=]() {        
        tryPasteRows();
    });
    contextMenu->exec(mapToGlobal(pos));
}
