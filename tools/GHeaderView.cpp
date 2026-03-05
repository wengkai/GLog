#include "GHeaderView.h"
#include "record.h"
#include <QMenu>
#include <QString>
#include <QMouseEvent>
#include <QApplication>
#include <QClipboard>
#include <string>
#include <cstdlib>
#include <cstring>

GHeaderView::GHeaderView(Qt::Orientation orientation, QWidget *parent) : QHeaderView(orientation, parent)
{
    setSectionsClickable(true);
    setHighlightSections(true);
    setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
    connect(this, &GHeaderView::customContextMenuRequested, this, &GHeaderView::customContextMenu);
}

void GHeaderView::customContextMenu(const QPoint& pos) 
{
    int section = logicalIndexAt(pos);
    if (section != -1) {
        auto contextMenu = new QMenu(this);
        QAction *actionSortAscending = contextMenu->addAction(tr("sort by column (ascending)"));
        connect(actionSortAscending, &QAction::triggered, this, [=](){
            emit sortByColumnSignal(section, Qt::AscendingOrder);
        });
        QAction* actionSortDescending = contextMenu->addAction(tr("sort by column (descending)"));
        connect(actionSortDescending, &QAction::triggered, this, [=](){
            emit sortByColumnSignal(section, Qt::DescendingOrder);
        });
        QAction* actionCopy = contextMenu->addAction(tr("copy column name"));
        connect(actionCopy, &QAction::triggered, this, [=](){
            QApplication::clipboard()->setText(model()->headerData(section, orientation()).toString());
        });
        if (section >= GRecord::RESOLVE_HEADERS_COUNT) {
            QAction *actionDelete = contextMenu->addAction(tr("delete column"));
            connect(actionDelete, &QAction::triggered, this, [=]() {
                emit removeColumnSignal(section);
            });
        }
        contextMenu->exec(mapToGlobal(pos));
    }
}
