#ifndef DROPABLETABLEVIEW_H
#define DROPABLETABLEVIEW_H

#include "GHeaderView.h"
#include <QTableView>
#include <QMimeData>
#include <QItemSelection>

class DropAbleTableView : public QTableView
{
    Q_OBJECT

private:
    GHeaderView* headerview;

public:
    explicit DropAbleTableView(QWidget *parent = nullptr);
    void tryDeleteSelectedRows();
    void tryNewSelectedRowsView();
    void tryPasteRows();
    void tryCopySelectedRows();
    GHeaderView* getHeaderView();

public slots:
    void customContextMenu(const QPoint& pos);
    void findNext(QString key, QString value, bool isReg);
    void foundNext(QModelIndex index);
    void selectRows(QList<int> rows);
    void deselectRows(QList<int> rows);
    void setMSelection(QItemSelectionModel::SelectionFlag command);

signals:
    void deleteRowsSignal(QModelIndexList indexes);
    void newViewWithRowsSignal(QModelIndexList indexes);
    void pasteRowsSignal(const QMimeData* mimeData);
    void copyRowsSignal(const QModelIndexList indexes);
    void findNextSignal(QModelIndex current, QString key, QString value, bool isReg);
    void setMSelectionSignal(QItemSelectionModel::SelectionFlag command);
    void selected();

protected:
    void dropEvent(QDropEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QItemSelection m_selection;

};

#endif