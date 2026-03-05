#ifndef SEARCHBAR_H
#define SEARCHBAR_H

#include <QDialog>
#include <QModelIndex>
#include "ui_SearchBar.h"

class SearchBar : public QDialog
{
    Q_OBJECT

public:
    explicit SearchBar(QWidget *parent = nullptr);
    ~SearchBar();

public slots:
    void disableButtons();
    void enableButtons();

signals:
    void findNext(QString key, QString value, bool isReg);
    void selectAll(QString key, QString value, bool isReg);
    void deselectAll(QString key, QString value, bool isReg);

private:
    Ui::SearchBarClass ui;

};

#endif