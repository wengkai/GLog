#ifndef GLOG_UI_H
#define GLOG_UI_H

#include <QtWidgets/QMainWindow>
#include <QTableView>
#include <QThread>
#include "adifdb.h"
#include "glogparser.h"
#include "ui_GLogApplication.h"

class ParserThread : public QObject 
{
    Q_OBJECT

public:
    explicit ParserThread(QObject *parent = nullptr);

public slots:
    void openFile(QString filename, AdifModel* model);
    void appendFile(QString filename, AdifModel* model);

signals:
    void done();

private:
    GLOG_PARSER::GLogParserDriver driver{  };
    GLOG_PARSER::Parser parser{ &driver };

};

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
    void openFileActionSignal(QString filename, AdifModel* model);
    void openFileAppendActionSignal(QString filename, AdifModel* model);

private:
    Ui::GLogApplicationClass ui;
    QTableView* tableview = nullptr;
    QThread parserSub;
    AdifModel* model = nullptr;
    
};

#endif