#ifndef DROPABLETABLEVIEW_H
#define DROPABLETABLEVIEW_H

#include <QTableView>

class DropAbleTableView : public QTableView
{
    Q_OBJECT

public:
    explicit DropAbleTableView(QWidget *parent = nullptr);
    void tryDeleteSelectedRows();

signals:
    void deleteRows(QModelIndexList indexes);

protected:
    void dropEvent(QDropEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;


};

#endif