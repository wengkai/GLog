#ifndef GHEADERVIEW_H
#define GHEADERVIEW_H

#include <QHeaderView>

class GHeaderView : public QHeaderView 
{
    Q_OBJECT

public:
    explicit GHeaderView(Qt::Orientation orientation = Qt::Orientation::Horizontal, QWidget *parent = nullptr);

public slots:
    void customContextMenu(const QPoint& pos);

signals:
    void sortByColumnSignal(int section, Qt::SortOrder order);
    void removeColumnSignal(int section);


};

#endif