#ifndef GLOG_UI_H
#define GLOG_UI_H

#include <QtWidgets/QMainWindow>
#include <QTableView>
#include <QThread>
#include "adifdb.h"
#include "glogparser.h"
#include "DropAbleTableView.h"
#include "ui_GLogApplication.h"


class GLogApplication : public QMainWindow
{
    Q_OBJECT

public:
    GLogApplication(QWidget *parent = nullptr);
    ~GLogApplication();

    void resizeTableView();
    void openFile(const QString& filename);
    void openFileAppend(const QString& filename);
    
public slots:
    void openFileAction();
    void openFileAppendAction();
    void modelUpdated();

signals:
    void openFileActionSignal(QString filename);
    void openFileAppendActionSignal(QString filename);

private:
    Ui::GLogApplicationClass ui;
    DropAbleTableView* tableview = nullptr;
    QThread modelSub;
    AdifModel* model = nullptr;
    
};

#endif