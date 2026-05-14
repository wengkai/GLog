#ifndef GLOG_UI_H
#define GLOG_UI_H

#include <QFuture>
#include <QLibrary>
#include <QList>
#include <QMessageBox>
#include <QMimeData>
#include <QPointer>
#include <QTableView>
#include <QTimer>
#include <QtResource>
#include <QtWidgets/QMainWindow>
#include "AddQSODialog.h"
#include "ConfigureCtyDialog.h"
#include "DropAbleTableView.h"
#include "DuplicatesManager.h"
#include "GlobalNetwork.h"
#include "MapWidget.h"
#include "SearchBar.h"
#include "adifdb.h"
#include "app_export.h"

inline void initAssetsResource() { Q_INIT_RESOURCE(assets); }

class FIFOBackendThreadExecutor;
class AwardPluginManager;
class AdifFileService;
class StdinReaderWorker;

namespace Ui {
class GLogApplicationClass;
};

class GLOGKIT_API GLogApplication : public QMainWindow {
    Q_OBJECT

    static constexpr int DEFAULT_MSG_TIMEOUT = 5000;
    bool m_msgBoxEnabled = false;

  public:
    GLogApplication(bool msgBoxEnabled = false, QWidget *parent = nullptr);
    ~GLogApplication();

    void resizeTableView();
    QFuture<std::vector<std::string>> openFile(const QString &filename);
    void mergeFile(const QString &filename, bool remove = false);
    void saveAsFile(const QString &filename);
    void initCtyDB(bool show_warning = true);
    static CtyDB *getCtyDBInstance();

    void enableBackup();
    void enableMsgBox();

  public slots:
    void openFileAction();
    void mergeFileAction();
    void modelUpdated();
    void saveAsAction();
    void saveDone();
    void setCilpboard(QMimeData *mimeData);
    void manageAwardPluginAction();
    void moveDataFromStdin();

  private slots:
    void onMapCallSignInViewTriggered();
    void onMapCallSignInViewFinished(int failCount, int confictCount);

  signals:
    void mergeFileActionSignal(QString filename, bool remove = false);
    void saveAsActionSignal(QString filename);
    void information(QString title, QString text, QMessageBox::StandardButton button0,
                     QMessageBox::StandardButton button1 = QMessageBox::StandardButton::NoButton);
    void warning(QString title, QString text, QMessageBox::StandardButton button0,
                 QMessageBox::StandardButton button1 = QMessageBox::StandardButton::NoButton);
    void critical(QString title, QString text, QMessageBox::StandardButton button0,
                  QMessageBox::StandardButton button1 = QMessageBox::StandardButton::NoButton);
    void showMessage(QString message, int timeout = 0);
    void disableAction(QAction *action);
    void enableAction(QAction *action);
    void initCtyDBDone();
    /** Microseconds spent in AwardEntityCountReport::format for the last Award action (emitted
     * before the Award information() dialog). */
    void perfAwardReportFinished(qint64 elapsedUs);

  private:
    void mapCallSignAskOverwritePreference();

    Ui::GLogApplicationClass *ui;
    DropAbleTableView *tableview = nullptr;
    AdifModel *model = nullptr;
    AdifFileService *fileService = nullptr;
    SearchBar *searchBar = nullptr;
    MapGraphicsView *mapView = nullptr;
    ConfigureCtyDialog *configureCtyDialog = nullptr;
    AddQSODialog *addQSODialog = nullptr;
    DuplicatesManager *duplicatesManager = nullptr;
    AwardPluginManager *awardPluginManager = nullptr;

    // std::vector<LoadedAwardPlugin> m_award_plugins;
    std::shared_ptr<StdinReaderWorker> stdinReader;
    QTimer stdinReaderTimer;
};

#endif