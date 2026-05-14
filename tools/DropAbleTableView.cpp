#include "DropAbleTableView.h"
#include <QAbstractItemModel>
#include <QApplication>
#include <QClipboard>
#include <QDropEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPointer>
#include <algorithm>
#include <exception>

#include "Concurrent.h"
#include "GHeaderView.h"
#include "MultiLineDelegate.h"
#include "adifdb.h"

namespace {

/**
 * Turn a list of row indices into disjoint [first,last] intervals (sorted, unique, in-range).
 * Runs on a worker thread; only touches plain integers.
 */
QList<QPair<int, int>> compressedRowRanges(QVector<int> rows, int rowCount) {
    if (rowCount <= 0 || rows.isEmpty()) {
        return {};
    }
    std::sort(rows.begin(), rows.end());
    rows.erase(std::unique(rows.begin(), rows.end()), rows.end());

    QList<QPair<int, int>> ranges;
    int start = -1;
    int prev = -1;
    for (int r : rows) {
        if (r < 0 || r >= rowCount) {
            continue;
        }
        if (start < 0) {
            start = prev = r;
            continue;
        }
        if (r == prev + 1) {
            prev = r;
            continue;
        }
        ranges.append({start, prev});
        start = prev = r;
    }
    if (start >= 0) {
        ranges.append({start, prev});
    }
    return ranges;
}

} // namespace

DropAbleTableView::DropAbleTableView(AdifModel *model, QWidget *parent)
    : m_model(model), QTableView(parent) {
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
    connect(this, &DropAbleTableView::customContextMenuRequested, this,
            &DropAbleTableView::customContextMenu);

    setAdifModel(model);
}

void DropAbleTableView::setAdifModel(AdifModel *model) {
    setModel(model);
    m_model = model;
    auto *tableview = this;
    connect(tableview->getHeaderView(), &GHeaderView::sortByColumnSignal, model,
            &AdifModel::sortSelectedColumn);
    connect(tableview->getHeaderView(), &GHeaderView::removeColumnSignal, model,
            &AdifModel::removeSelectedColumn);
    connect(tableview, &DropAbleTableView::findNextSignal, model, &AdifModel::findNextS);
    connect(model, &AdifModel::foundNext, tableview, &DropAbleTableView::foundNext);
    connect(model, &AdifModel::selectRows, tableview, &DropAbleTableView::selectRows);
    connect(model, &AdifModel::deselectRows, tableview, &DropAbleTableView::deselectRows);
}

void DropAbleTableView::tryDeleteSelectedRows() {
    auto rows = selectionModel()->selectedRows();
    if (rows.empty()) {
        return;
    }
    auto button =
        QMessageBox::question(this, tr("Delete"), tr("Delete selected row(s)?"),
                              QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No);
    if (button == QMessageBox::StandardButton::Yes) {
        proceedDeleteSelectedRows(rows);
    }
}

void DropAbleTableView::proceedDeleteSelectedRows(const QModelIndexList &rows) {
    m_model->deleteRows(rows);
}

void DropAbleTableView::tryNewSelectedRowsView() {
    auto rows = selectionModel()->selectedRows();
    if (rows.empty()) {
        return;
    }
    auto button =
        QMessageBox::question(this, tr("New view"), tr("Create new view with selected row(s)?"),
                              QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No);
    if (button == QMessageBox::StandardButton::Yes) {
        proceedNewViewWithRows(rows);
    }
}

void DropAbleTableView::proceedNewViewWithRows(const QModelIndexList &rows) {
    m_model->newViewWithRows(rows);
}

void DropAbleTableView::tryPasteRows() {
    auto *clipboard = QGuiApplication::clipboard();
    const auto *mimeData = clipboard->mimeData();
    if (mimeData == nullptr) {
        return;
    }
    auto button =
        QMessageBox::question(this, tr("Paste"), tr("Paste rows from clipboard?"),
                              QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No);
    if (button == QMessageBox::StandardButton::Yes) {
        proceedPasteRows(mimeData);
    }
}

void DropAbleTableView::proceedPasteRows(const QMimeData *mimeData) {
    m_model->pasteRows(mimeData);
}

void DropAbleTableView::tryCopySelectedRows() {
    auto rows = selectionModel()->selectedRows();
    if (!rows.empty()) {
        m_model->copyRows(rows);
    }
}

auto DropAbleTableView::getHeaderView() -> GHeaderView * { return headerview; }

void DropAbleTableView::findNext(const QString &key, const QString &value, bool isReg) {
    if (model()->rowCount() == 0) {
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

void DropAbleTableView::foundNext(QModelIndex index) {
    if (index.isValid()) {
        selectionModel()->setCurrentIndex(index,
                                          QItemSelectionModel::SelectionFlag::ClearAndSelect);
        scrollTo(index, QAbstractItemView::ScrollHint::PositionAtCenter);
    } else {
        emit userInformation(tr("Find"), tr("No more matching record found."));
    }
    emit selected();
}

void DropAbleTableView::applySelectionOnGuiThread(QItemSelection selection,
                                                  QItemSelectionModel::SelectionFlag command) {
    if (selectionModel() == nullptr) {
        return;
    }
    setUpdatesEnabled(false);
    selectionModel()->select(selection, command);
    setUpdatesEnabled(true);
    emit selected();
}

void DropAbleTableView::applyRowRangeSelectionAsync(
    const QList<int> &rows, QItemSelectionModel::SelectionFlags mergeFlags) {
    QAbstractItemModel *m = model();
    const int rowCount = m ? m->rowCount() : 0;
    const QItemSelection baseSelection =
        selectionModel() ? selectionModel()->selection() : QItemSelection{};

    QVector<int> rowVec;
    rowVec.reserve(rows.size());
    for (int r : rows) {
        rowVec.push_back(r);
    }

    GLogConcurrent::makeFuture(
        [rowVec, rowCount]() mutable { return compressedRowRanges(std::move(rowVec), rowCount); })
        .then(this, [self = QPointer<DropAbleTableView>(this), baseSelection,
                     mergeFlags](QList<QPair<int, int>> ranges) {
            if (!self) {
                return;
            }
            if (self->selectionModel() == nullptr || self->model() == nullptr) {
                return;
            }
            const int columnCount = self->model()->columnCount();
            if (columnCount <= 0) {
                return;
            }
            const int nRows = self->model()->rowCount();
            const int lastCol = columnCount - 1;
            QItemSelection acc = baseSelection;
            for (const auto &ab : ranges) {
                if (nRows <= 0) {
                    break;
                }
                const int a = std::clamp(ab.first, 0, nRows - 1);
                const int b = std::clamp(ab.second, 0, nRows - 1);
                const QModelIndex topLeft = self->model()->index(a, 0);
                const QModelIndex bottomRight = self->model()->index(b, lastCol);
                if (!topLeft.isValid() || !bottomRight.isValid()) {
                    continue;
                }
                acc.merge(QItemSelection(topLeft, bottomRight), mergeFlags);
            }
            self->applySelectionOnGuiThread(acc, QItemSelectionModel::ClearAndSelect);
        });
}

void DropAbleTableView::selectRows(const QList<int> &rows) {
    applyRowRangeSelectionAsync(rows, QItemSelectionModel::Select);
}

void DropAbleTableView::deselectRows(const QList<int> &rows) {
    applyRowRangeSelectionAsync(rows, QItemSelectionModel::Deselect);
}

void DropAbleTableView::dropEvent(QDropEvent *event) { QTableView::dropEvent(event); }

void DropAbleTableView::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
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
    auto *contextMenu = new QMenu(this);
    if (index.isValid()) {
        if (!selectionModel()->selectedRows().empty()) {
            QAction *actionDelete = contextMenu->addAction(tr("delete row(s)"));
            connect(actionDelete, &QAction::triggered, this, [=]() { tryDeleteSelectedRows(); });
            QAction *actionNewView = contextMenu->addAction(tr("new view with row(s)"));
            connect(actionNewView, &QAction::triggered, this, [=]() { tryNewSelectedRowsView(); });
            QAction *actionCopy = contextMenu->addAction(tr("copy row(s)"));
            connect(actionCopy, &QAction::triggered, this, [=]() { tryCopySelectedRows(); });
        }
        QAction *actionCopyCell = contextMenu->addAction(tr("copy cell content"));
        connect(actionCopyCell, &QAction::triggered, this, [=]() {
            auto data = model()->data(index).toString();
            QApplication::clipboard()->setText(data);
        });
    }
    QAction *actionPaste = contextMenu->addAction(tr("paste"));
    connect(actionPaste, &QAction::triggered, this, [=]() { tryPasteRows(); });
    contextMenu->exec(mapToGlobal(pos));
}
