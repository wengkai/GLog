#ifndef GLOG_UI_H
#define GLOG_UI_H

#include <QFuture>
#include <QMessageBox>
#include <QMimeData>
#include <QTableView>
#include <QtResource>
#include <QtWidgets/QMainWindow>
#include "AddQSODialog.h"
#include "ConfigureCtyDialog.h"
#include "DropAbleTableView.h"
#include "GlobalNetwork.h"
#include "MapWidget.h"
#include "SearchBar.h"
#include "adifdb.h"
#include "app_export.h"
#include "glogparser.h"

#include "award_plugin.h"

inline void initAssetsResource() { Q_INIT_RESOURCE(assets); }

namespace Ui {
class GLogApplicationClass;
};

class APP_EXPORT GLogApplication : public QMainWindow {
    Q_OBJECT

    static constexpr int DEFAULT_MSG_TIMEOUT = 5000;

  public:
    GLogApplication(QWidget *parent = nullptr);
    ~GLogApplication();

    void resizeTableView();
    QFuture<std::vector<std::string>> openFile(const QString &filename);
    void mergeFile(const QString &filename, bool remove = false);
    void saveAsFile(const QString &filename);
    static void extractZip(const QString &zipPath, const QString &extractDir,
                           QNetworkAccessManager &manager);
    void initCtyDB(bool show_warning = true);
    static CtyDB *getCtyDBInstance();

  public slots:
    void openFileAction();
    void mergeFileAction();
    void modelUpdated();
    void saveAsAction();
    void saveDone();
    void setCilpboard(QMimeData *mimeData);
    void updateFccDatabase();

  signals:
    void mergeFileActionSignal(QString filename, bool remove = false);
    void saveAsActionSignal(QString filename);
    void information(QString title, QString text, QMessageBox::StandardButton button0,
                     QMessageBox::StandardButton button1 = QMessageBox::StandardButton::NoButton);
    void warning(QString title, QString text, QMessageBox::StandardButton button0,
                 QMessageBox::StandardButton button1 = QMessageBox::StandardButton::NoButton);
    void showMessage(QString message, int timeout = 0);
    void disableAction(QAction *action);
    void enableAction(QAction *action);
    void initCtyDBDone();

  private:
    Ui::GLogApplicationClass *ui;
    DropAbleTableView *tableview = nullptr;
    AdifModel *model = nullptr;
    SearchBar *searchBar = nullptr;
    MapGraphicsView *mapView = nullptr;
    ConfigureCtyDialog *configureCtyDialog = nullptr;
    AddQSODialog *addQSODialog = nullptr;
};

#endif