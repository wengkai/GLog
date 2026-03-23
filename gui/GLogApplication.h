#ifndef GLOG_UI_H
#define GLOG_UI_H

#include <QtWidgets/QMainWindow>
#include <QTableView>
#include <QMimeData>
#include <QMessageBox>
#include "adifdb.h"
#include "glogparser.h"
#include "DropAbleTableView.h"
#include "SearchBar.h"
#include "MapWidget.h"
#include "ConfigureCtyDialog.h"
#include "AddQSODialog.h"
#include "GlobalNetwork.h"
#include "ui_GLogApplication.h"


class GLogApplication : public QMainWindow
{
    Q_OBJECT

public:
    GLogApplication(QWidget *parent = nullptr);
    ~GLogApplication();

    void resizeTableView();
    void openFile(const QString& filename);
    void mergeFile(const QString& filename, bool remove = false);
    void saveAsFile(const QString& filename);
    void extractZip(const QString & zipPath, const QString & extractDir, QNetworkAccessManager & manager);
    
public slots:
    void openFileAction();
    void mergeFileAction();
    void modelUpdated();
    void saveAsAction();
    void saveDone();
    void setCilpboard(QMimeData* mimeData);
    void updateFccDatabase();

signals:
    void openFileActionSignal(QString filename);
    void mergeFileActionSignal(QString filename, bool remove = false);
    void saveAsActionSignal(QString filename);
    void information(QString title,
                     QString text,
                     QMessageBox::StandardButton button0, QMessageBox::StandardButton button1 = QMessageBox::StandardButton::NoButton);
    void warning(QString title,
                     QString text,
                     QMessageBox::StandardButton button0, QMessageBox::StandardButton button1 = QMessageBox::StandardButton::NoButton);
    void showMessage(QString message, int timeout = 0);
    void disableAction(QAction * action);
    void enableAction(QAction * action);

private:
    Ui::GLogApplicationClass ui;
    DropAbleTableView* tableview = nullptr;
    AdifModel* model = nullptr;
    SearchBar* searchBar = nullptr;
    MapGraphicsView* mapView = nullptr;
    ConfigureCtyDialog* configureCtyDialog = nullptr;
    AddQSODialog* addQSODialog = nullptr;
    
};

#endif