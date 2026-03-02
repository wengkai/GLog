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
    void mergeFile(const QString& filename);
    void saveAsFile(const QString& filename);
    
public slots:
    void openFileAction();
    void mergeFileAction();
    void modelUpdated();
    void saveAsAction();
    void saveDone();

signals:
    void openFileActionSignal(QString filename);
    void mergeFileActionSignal(QString filename);
    void saveAsActionSignal(QString filename);

private:
    Ui::GLogApplicationClass ui;
    DropAbleTableView* tableview = nullptr;
    QThread modelSub;
    AdifModel* model = nullptr;
    
};

#endif