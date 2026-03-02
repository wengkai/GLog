#ifndef DROPABLETABLEVIEW_H
#define DROPABLETABLEVIEW_H

#include "GHeaderView.h"
#include <QTableView>

class DropAbleTableView : public QTableView
{
    Q_OBJECT

private:
    GHeaderView* headerview;

public:
    explicit DropAbleTableView(QWidget *parent = nullptr);
    void tryDeleteSelectedRows();
    GHeaderView* getHeaderView();

signals:
    void deleteRows(QModelIndexList indexes);

protected:
    void dropEvent(QDropEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;


};

#endif