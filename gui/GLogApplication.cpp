#include "GLogApplication.h"
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFunctionPointer>
#include <QLabel>
#include <QLibrary>
#include <QLibraryInfo>
#include <QMessageBox>
#include <QOpenGLWidget>
#include <QSortFilterProxyModel>
#include <QStandardPaths>
#include <QTimer>
#include <QVBoxLayout>
#include <fstream>
#include <utility>
#include "AwardPluginManager.h"
#include "Concurrent.h"
#include "GCommandLineParser.h"
#include "MultiLineDelegate.h"
#include "Synchronize.h"
#include "adiffile_service.h"
#include "adifparse_service.h"
#include "award_entity_count_report.h"
#include "ctydb.h"
#include "recordrepository.h"
#include "ui_GLogApplication.h"

GLogApplication::GLogApplication(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::GLogApplicationClass()) {
    ui->setupUi(this);

    connect(ui->actionAbout_Qt, &QAction::triggered, this, []() { QApplication::aboutQt(); });

    awardPluginManager = new AwardPluginManager(this);
    connect(ui->actionManage_Award_Plugin, &QAction::triggered, this,
            &GLogApplication::manageAwardPluginAction);
    connect(
        awardPluginManager, &AwardPluginManager::userInformation, this,
        [this](const QString &title, const QString &text, QMessageBox::StandardButton button0,
               QMessageBox::StandardButton button1) {
            emit information(title, text, button0, button1);
        },
        Qt::QueuedConnection);
    connect(
        awardPluginManager, &AwardPluginManager::userCritical, this,
        [this](const QString &title, const QString &text, QMessageBox::StandardButton button0,
               QMessageBox::StandardButton button1) {
            emit critical(title, text, button0, button1);
        },
        Qt::QueuedConnection);

    connect(
        this, &GLogApplication::information, this,
        [=](const QString &title, const QString &text, QMessageBox::StandardButton button0,
            QMessageBox::StandardButton button1) {
            QMessageBox::information(this, title, text, button0, button1);
        },
        Qt::QueuedConnection);

    connect(
        this, &GLogApplication::warning, this,
        [=](const QString &title, const QString &text, QMessageBox::StandardButton button0,
            QMessageBox::StandardButton button1) {
            QMessageBox::warning(this, title, text, button0, button1);
        },
        Qt::QueuedConnection);

    connect(
        this, &GLogApplication::critical, this,
        [=](const QString &title, const QString &text, QMessageBox::StandardButton button0,
            QMessageBox::StandardButton button1) {
            QMessageBox::critical(this, title, text, button0, button1);
        },
        Qt::QueuedConnection);

    connect(
        this, &GLogApplication::showMessage, this,
        [=](const QString &message, int timeout) { ui->statusBar->showMessage(message, timeout); },
        Qt::QueuedConnection);

    connect(
        this, &GLogApplication::enableAction, this,
        [=](QAction *action) { action->setEnabled(true); }, Qt::QueuedConnection);

    connect(
        this, &GLogApplication::disableAction, this,
        [=](QAction *action) { action->setEnabled(false); }, Qt::QueuedConnection);

    model = new AdifModel(this);
    {
        const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(appData);
        const QString dbPath = appData + QStringLiteral("/glog_records.sqlite");
        auto recordRepo = std::make_shared<GRecordRepository>(dbPath);
        recordRepo->initSchemaAsync()
            .then(this, [model = model, recordRepo]() { model->setDbBackup(recordRepo); })
            .onFailed(this, [](const std::exception &e) {
                qCritical() << "GRecordRepository initSchemaAsync failed:" << e.what();
            });
    }
    tableview = new DropAbleTableView(model, this);
    setCentralWidget(tableview);
    connect(ui->actionOpen, &QAction::triggered, this, &GLogApplication::openFileAction);
    connect(ui->actionMerge, &QAction::triggered, this, &GLogApplication::mergeFileAction);
    connect(ui->actionSave_As, &QAction::triggered, this, &GLogApplication::saveAsAction);

    fileService = new AdifFileService(model, this);
    model->setFileService(fileService);
    connect(this, &GLogApplication::saveAsActionSignal, fileService, &AdifFileService::saveAs);
    connect(fileService, &AdifFileService::saveDone, this, &GLogApplication::saveDone,
            Qt::QueuedConnection);
    connect(
        fileService, &AdifFileService::saveFailed, this,
        [this](const QString &detail) {
            emit critical(tr("Save"), tr("Failed: ") + detail, QMessageBox::StandardButton::Ok);
        },
        Qt::QueuedConnection);
    connect(model, &AdifModel::setCilpboard, this, &GLogApplication::setCilpboard,
            Qt::QueuedConnection);

    connect(
        tableview, &DropAbleTableView::userInformation, this,
        [this](const QString &title, const QString &text) {
            emit information(title, text, QMessageBox::StandardButton::Ok);
        },
        Qt::QueuedConnection);

    GCommandLineParser parser;
    parser.process(QCoreApplication::arguments());
    bool remove = parser.isSet("remove");
    if (parser.isSet("i")) {
        auto files = parser.values("i");
        for (auto &file : files) {
            mergeFile(file, remove);
        }
    }

    searchBar = new SearchBar(this);
    connect(ui->actionSearch, &QAction::triggered, searchBar, &SearchBar::show);
    connect(searchBar, &SearchBar::findNext, tableview, &DropAbleTableView::findNext);
    connect(searchBar, &SearchBar::selectAll, model, &AdifModel::selectAll);
    connect(searchBar, &SearchBar::deselectAll, model, &AdifModel::deselectAll);
    connect(tableview, &DropAbleTableView::selected, searchBar, &SearchBar::enableButtons);

    auto *mapWidget = new MapWidget(model, this);
    connect(ui->actionMap_View, &QAction::triggered, mapWidget, &MapWidget::show);
    connect(ui->actionMap_View, &QAction::triggered, mapWidget, &MapWidget::dataVisualize);
    duplicatesManager = new DuplicatesManager(model, this);
    connect(ui->actionManage_Duplicates, &QAction::triggered, duplicatesManager,
            &DuplicatesManager::exec);
    connect(
        duplicatesManager, &DuplicatesManager::userInformation, this,
        [this](const QString &title, const QString &text) {
            emit information(title, text, QMessageBox::StandardButton::Ok);
        },
        Qt::QueuedConnection);

    auto *ctydb = CtyDB::instance();
    connect(ctydb, &CtyDB::dbHintChanged, mapWidget, &MapWidget::initCtyMarkers,
            Qt::QueuedConnection);

    configureCtyDialog = new ConfigureCtyDialog(this);
    connect(ctydb, &CtyDB::dbHintChanged, configureCtyDialog, &ConfigureCtyDialog::setDBhint,
            Qt::QueuedConnection);
    connect(mapWidget, &MapWidget::initCtyMarkersBegin, configureCtyDialog,
            &ConfigureCtyDialog::disableCtyConfigure, Qt::QueuedConnection);
    connect(mapWidget, &MapWidget::initCtyMarkersEnd, configureCtyDialog,
            &ConfigureCtyDialog::enableCtyConfigure, Qt::QueuedConnection);
    connect(model, &AdifModel::mapCallSignInViewBegin, configureCtyDialog,
            &ConfigureCtyDialog::disableCtyConfigure, Qt::QueuedConnection);
    connect(model, &AdifModel::mapCallSignInViewEnd, configureCtyDialog,
            &ConfigureCtyDialog::enableCtyConfigure, Qt::QueuedConnection);
    connect(ui->actionConfigure_Cty_dat, &QAction::triggered,
            [=]() { configureCtyDialog->exec(); });
    connect(
        configureCtyDialog, &ConfigureCtyDialog::loadFinishedInformation, this,
        [this](const QString &msg) {
            emit information(tr("Load"), msg, QMessageBox::StandardButton::Ok);
        },
        Qt::QueuedConnection);

    initCtyDB();

    GLogNetwork::init(this);

    connect(ui->actionMapCallSignInView, &QAction::triggered, this,
            &GLogApplication::onMapCallSignInViewTriggered);
    connect(model, &AdifModel::mapCallSignInViewEnd, this,
            &GLogApplication::onMapCallSignInViewFinished);

    addQSODialog = new AddQSODialog(model, this);
    connect(ui->actionAdd_QSO_Manually, &QAction::triggered, [=]() { addQSODialog->exec(); });
    connect(
        addQSODialog, &AddQSODialog::userInformation, this,
        [this](const QString &title, const QString &text) {
            emit information(title, text, QMessageBox::StandardButton::Ok);
        },
        Qt::QueuedConnection);
    connect(
        addQSODialog, &AddQSODialog::userWarning, this,
        [this](const QString &title, const QString &text) {
            emit warning(title, text, QMessageBox::StandardButton::Ok);
        },
        Qt::QueuedConnection);

    connect(ui->actionAward, &QAction::triggered, [=]() {
        model->snapshotAsync()
            .then([mgr = awardPluginManager](AdifModelSnapshot snap) {
                AwardEntityCountReport reporter(mgr);
                return reporter.format(snap.records);
            })
            .then(this, [=](QString result) {
                emit information(tr("Award"), std::move(result), QMessageBox::StandardButton::Ok);
            });
    });

    stdinReader = std::make_shared<StdinReaderWorker>();
    constexpr int STDIN_TIMER_INTERVAL = 1000;
    stdinReaderTimer.setInterval(STDIN_TIMER_INTERVAL);
    connect(&stdinReaderTimer, &QTimer::timeout, this, &GLogApplication::moveDataFromStdin);
    // 无执行器默认行为：投递到QThreadPool::globalInstance()
    GLogConcurrent::makeFuture([stdinReader = this->stdinReader]() { return stdinReader->run(); })
        .then(this,
              [=](int parse_ret) {
                  stdinReaderTimer.stop();
                  // 管道不可用或中断，静默返回
                  if (parse_ret == StdinReaderWorker::PipeUnavailable ||
                      parse_ret == StdinReaderWorker::Interrupt) {
                      return;
                  }
                  moveDataFromStdin();
                  auto count = stdinReader->errors.requireRead()->size();
                  // 其他错误码，提示用户解析异常终止+错误数量并返回
                  if (parse_ret != StdinReaderWorker::Success) {
                      emit critical(tr("Error"),
                                    tr("Parse exception terminated. %1 errors found.").arg(count),
                                    QMessageBox::StandardButton::Ok);
                      return;
                  }
                  // 出现一条以上的解析错误，警告错误数量
                  if (count > 0) {
                      emit warning(tr("Warning"), tr("%1 errors found").arg(count),
                                   QMessageBox::StandardButton::Ok);
                  }
              })
        .onFailed(this, [this](const std::exception &e) {
            stdinReaderTimer.stop();
            emit critical(tr("Error"), QString::fromUtf8(e.what()),
                          QMessageBox::StandardButton::Ok);
        });

    stdinReaderTimer.start();
}

void GLogApplication::moveDataFromStdin() {
    using DataType = std::remove_reference_t<decltype(stdinReader->data)>::MyType;
    DataType m_data;
    stdinReader->data.write([&](auto &data) { // 使用写代理移动数据
        std::swap(m_data, data);
    });
    if (m_data.empty()) {
        return;
    }
    // 投递到模型专用FIFOBackendThreadExecutor，确保插入顺序不变
    GLogConcurrent::makeFuture(
        [this, m_data = std::move(m_data)]() mutable {
            auto res = AdifParseService::fromRawData(true, std::move(m_data));
            model->applyInsertAt(-1, std::move(res));
        },
        *(model->getFIFOBackendThreadExecutor()));
}

GLogApplication::~GLogApplication() {
    QCoreApplication::processEvents();
    if (model != nullptr) {
        model->setFileService(nullptr);
    }
    stdinReaderTimer.stop();
    stdinReader->interrupt();
    QThreadPool::globalInstance()->waitForDone(); // 程序结束前的最后等待
    QCoreApplication::processEvents();
    delete ui;
}

void GLogApplication::resizeTableView() {
    tableview->setVisible(false);
    tableview->resizeColumnsToContents();
    tableview->resizeRowsToContents();
    tableview->setVisible(true);
}

auto GLogApplication::openFile(const QString &filename) -> QFuture<std::vector<std::string>> {
    return fileService->openFileAsync(filename);
}

void GLogApplication::openFileAction() {
    auto filename = QFileDialog::getOpenFileName(this, tr("Open File"), "",
                                                 tr("Adif Files (*.adi *.adif);;All Files (*.*)"));

    if (filename.isEmpty()) {
        return;
    }

    openFile(filename)
        .then(this,
              [this](const std::vector<std::string> &errors) {
                  if (!errors.empty()) {
                      emit warning(tr("Warning"), tr("%1 errors found").arg(errors.size()),
                                   QMessageBox::StandardButton::Ok);
                  }
              })
        .onFailed(this, [this](const std::exception &e) {
            emit critical(tr("Error"), QString::fromUtf8(e.what()),
                          QMessageBox::StandardButton::Ok);
        });
}

void GLogApplication::mergeFile(const QString &filename, bool remove) {
    fileService->insertFileAsync(-1, filename, remove);
}

void GLogApplication::saveAsFile(const QString &filename) { emit saveAsActionSignal(filename); }

void GLogApplication::initCtyDB(bool show_warning) {
    qDebug() << "Checking DB initialization. Object Address:" << this
             << " | Thread:" << QThread::currentThread();

    auto *ctydb = CtyDB::instance();
    configureCtyDialog->disableCtyConfigure();
    GLogConcurrent::makeFuture([=]() {
        QEventLoop loop;
        QNetworkAccessManager manager;
        QNetworkRequest req(QUrl(QString::fromLatin1(CtyDB::DEFAULT_ONLINE_DATA)));
        GLogNetwork::setGeneralHeader(req);
        auto *rep = manager.get(req);
        QObject::connect(rep, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        if (rep->error() == QNetworkReply::NoError) {
            auto cty = QString(rep->readAll());
            ctydb->loadDBString(cty, CtyDB::DEFAULT_ONLINE_DATA);
        } else {
            if (show_warning) {
                emit warning(tr("Warning"),
                             tr("Can not fetch cty.dat online. Using build-in one.\nYou should "
                                "consider to check your network, or add a cty.dat manually.\n") +
                                 rep->errorString(),
                             QMessageBox::StandardButton::Ok);
            }
            ctydb->loadLocalDB();
        }
        rep->deleteLater();
    }).then(this, [=]() {
        configureCtyDialog->enableCtyConfigure();
        emit initCtyDBDone();
    });
}

auto GLogApplication::getCtyDBInstance() -> CtyDB * { return CtyDB::instance(); }

void GLogApplication::mergeFileAction() {
    mergeFile(QFileDialog::getOpenFileName(this, tr("Open File"), "",
                                           tr("Adif Files (*.adi *.adif);;All Files (*.*)")));
}

void GLogApplication::modelUpdated() {
    // resizeTableView();
    // tableview->setVisible(true);
}

void GLogApplication::saveAsAction() {
    if (model->rowCount() == 0) {
        return;
    }
    if (auto filename = QFileDialog::getSaveFileName(
            this, tr("Save As"), "", tr("Adif Files (*.adi *.adif);;Csv Files (*.csv)"));
        !filename.isEmpty()) {
        saveAsFile(filename);
    }
}

void GLogApplication::saveDone() {
    emit information(tr("Save"), tr("Done"), QMessageBox::StandardButton::Ok);
}

void GLogApplication::setCilpboard(QMimeData *mimeData) {
    QApplication::clipboard()->setMimeData(mimeData);
    emit information(tr("Copy"), tr("Done"), QMessageBox::StandardButton::Ok);
}

void GLogApplication::manageAwardPluginAction() {
    if (awardPluginManager) {
        awardPluginManager->exec();
    }
}

void GLogApplication::onMapCallSignInViewTriggered() {
    if (!CtyDB::instance()->ready()) {
        emit warning(tr("Warning"), tr("The mapping database has not ready."),
                     QMessageBox::StandardButton::Ok);
        return;
    }
    const auto button1 =
        QMessageBox::question(this, tr("Warning"),
                              tr("This action will update the following fields: "
                                 "\nCQZ, ITUZ, CONT, COUNTRY\nContinue?"),
                              QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No);
    if (button1 == QMessageBox::StandardButton::No) {
        return;
    }
    mapCallSignAskOverwritePreference();
}

void GLogApplication::mapCallSignAskOverwritePreference() {
    const auto button2 = QMessageBox::question(
        this, tr("Warning"),
        tr("Should we overwrite the following fields: \nCQZ, ITUZ, CONT, COUNTRY\nIf there is "
           "a confict to the original data?"),
        QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No);
    const bool keepOrigin = button2 == QMessageBox::StandardButton::No;
    model->mapCallSignInView(keepOrigin);
}

void GLogApplication::onMapCallSignInViewFinished(int failCount, int confictCount) {
    emit information(tr("Map Call Sign in View"),
                     tr("Done\n%1 record(s) failed to be mapped.\n%2 confict field(s) found.")
                         .arg(failCount)
                         .arg(confictCount),
                     QMessageBox::StandardButton::Ok);
}
