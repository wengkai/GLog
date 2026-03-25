#include "GLogApplication.h"
#include "ui_GLogApplication.h"
#include "MultiLineDelegate.h"
#include "GCommandLineParser.h"
#include "ctydb.h"
#include "Concurrent.h"
#include "fccdb.h"
#include <QVBoxLayout>
#include <QOpenGLWidget>
#include <QSortFilterProxyModel>
#include <QLabel>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QStandardPaths>
#include <QProcess>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDateTime>
#include <QTimer>
#include <fstream>

GLogApplication::GLogApplication(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::GLogApplicationClass())
{
    ui->setupUi(this);

    connect(this, &GLogApplication::information, this, [=](
        QString title,
        QString text,
        QMessageBox::StandardButton button0, QMessageBox::StandardButton button1){
            QMessageBox::information(this, title, text, button0, button1);
    }, Qt::QueuedConnection);

    connect(this, &GLogApplication::warning, this, [=](
        QString title,
        QString text,
        QMessageBox::StandardButton button0, QMessageBox::StandardButton button1){
            QMessageBox::warning(this, title, text, button0, button1);
    }, Qt::QueuedConnection);

    connect(this, &GLogApplication::showMessage, this, [=](QString message, int timeout){
        ui->statusBar->showMessage(message, timeout);
    }, Qt::QueuedConnection);

    connect(this, &GLogApplication::enableAction, this, [=](QAction * action){
        action->setEnabled(true);
    }, Qt::QueuedConnection);

    connect(this, &GLogApplication::disableAction, this, [=](QAction * action){
        action->setEnabled(false);
    }, Qt::QueuedConnection);

    tableview = new DropAbleTableView(this);
    setCentralWidget(tableview);
    connect(ui->actionOpen, &QAction::triggered, this, &GLogApplication::openFileAction);
    connect(ui->actionMerge, &QAction::triggered, this, &GLogApplication::mergeFileAction);
    connect(ui->actionSave_As, &QAction::triggered, this, &GLogApplication::saveAsAction);
    model = new AdifModel(this);
    tableview->setModel(model);
    connect(this, &GLogApplication::openFileActionSignal, model, &AdifModel::openFile);
    connect(this, &GLogApplication::mergeFileActionSignal, model, &AdifModel::appendFile);
    connect(this, &GLogApplication::saveAsActionSignal, model, &AdifModel::saveAs);
    connect(model, &AdifModel::saveDone, this, &GLogApplication::saveDone, Qt::QueuedConnection);
    connect(model, &AdifModel::setCilpboard, this, &GLogApplication::setCilpboard, Qt::QueuedConnection);
    connect(tableview, &DropAbleTableView::deleteRowsSignal, model, &AdifModel::deleteRows);
    connect(tableview, &DropAbleTableView::newViewWithRowsSignal, model, &AdifModel::newViewWithRows);
    connect(tableview, &DropAbleTableView::pasteRowsSignal, model, &AdifModel::pasteRows);
    connect(tableview, &DropAbleTableView::copyRowsSignal, model, &AdifModel::copyRows);
    connect(tableview->getHeaderView(), &GHeaderView::sortByColumnSignal, model, &AdifModel::sortSelectedColumn);
    connect(tableview->getHeaderView(), &GHeaderView::removeColumnSignal, model, &AdifModel::removeSelectedColumn);
    
    GCommandLineParser parser;
    parser.process(QCoreApplication::arguments());
    bool remove = parser.isSet("remove");
    if (parser.isSet("i")) {
        auto files = parser.values("i");
        for (auto& file: files) {
            //QMessageBox::information(this, "pre open", file, QMessageBox::StandardButton::Ok);
            mergeFile(file, remove);
        }
    }
    
    searchBar = new SearchBar(this);
    connect(ui->actionSearch, &QAction::triggered, searchBar, &SearchBar::show);
    connect(searchBar, &SearchBar::findNext, tableview, &DropAbleTableView::findNext);
    connect(searchBar, &SearchBar::selectAll, model, &AdifModel::selectAll);
    connect(searchBar, &SearchBar::deselectAll, model, &AdifModel::deselectAll);
    connect(tableview, &DropAbleTableView::selected, searchBar, &SearchBar::enableButtons);
    connect(tableview, &DropAbleTableView::findNextSignal, model, &AdifModel::findNextS);
    connect(model, &AdifModel::foundNext, tableview, &DropAbleTableView::foundNext);
    connect(model, &AdifModel::selectRows, tableview, &DropAbleTableView::selectRows);
    connect(model, &AdifModel::deselectRows, tableview, &DropAbleTableView::deselectRows);

    auto mapWidget = new MapWidget(model, this);
    connect(ui->actionMap_View, &QAction::triggered, mapWidget, &MapWidget::show);
    connect(ui->actionMap_View, &QAction::triggered, mapWidget, &MapWidget::dataVisualize);

    auto ctydb = CtyDB::instance();
    connect(ctydb, &CtyDB::dbHintChanged, mapWidget, &MapWidget::initCtyMarkers, Qt::QueuedConnection);

    configureCtyDialog = new ConfigureCtyDialog(this);
    connect(ctydb, &CtyDB::dbHintChanged, configureCtyDialog, &ConfigureCtyDialog::setDBhint, Qt::QueuedConnection);
    connect(mapWidget, &MapWidget::initCtyMarkersBegin, configureCtyDialog, &ConfigureCtyDialog::disableCtyConfigure, Qt::QueuedConnection);
    connect(mapWidget, &MapWidget::initCtyMarkersEnd, configureCtyDialog, &ConfigureCtyDialog::enableCtyConfigure, Qt::QueuedConnection);
    connect(model, &AdifModel::mapCallSignInViewBegin, configureCtyDialog, &ConfigureCtyDialog::disableCtyConfigure, Qt::QueuedConnection);
    connect(model, &AdifModel::mapCallSignInViewEnd, configureCtyDialog, &ConfigureCtyDialog::enableCtyConfigure, Qt::QueuedConnection);
    connect(ui->actionConfigure_Cty_dat, &QAction::triggered, [=](){
        configureCtyDialog->exec();
    });

    initCtyDB();

    GLogNetwork::init(this);

    connect(ui->actionMapCallSignInView, &QAction::triggered, [=](){
        if (!CtyDB::instance()->ready()) {
            QMessageBox::warning(this, tr("Warning"), tr("The mapping database has not ready."), QMessageBox::StandardButton::Ok);
            return;
        }
        auto button1 = QMessageBox::question(this, tr("Warning"), tr("This action will update the following fields: \nCQZ, ITUZ, CONT, COUNTRY\nContinue?"), QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No);
        if (button1 == QMessageBox::StandardButton::No) {
            return;
        }
        auto button2 = QMessageBox::question(this, tr("Warning"), tr("Should we overwrite the following fields: \nCQZ, ITUZ, CONT, COUNTRY\nIf there is a confict to the original data?"), QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No);
        bool keepOrigin = button2 == QMessageBox::StandardButton::No;
        model->mapCallSignInView(keepOrigin);
    });
    connect(model, &AdifModel::mapCallSignInViewEnd, [=](int failCount, int confictCount){
        QMessageBox::information(this, tr("Map Call Sign in View"), tr("Done\n%1 record(s) failed to be mapped.\n%2 confict field(s) found.").arg(failCount).arg(confictCount), QMessageBox::StandardButton::Ok);
    });

    addQSODialog = new AddQSODialog(model, this);
    connect(ui->actionAdd_QSO_Manually, &QAction::triggered, [=](){
        addQSODialog->exec();
    });

    connect(ui->actionAward, &QAction::triggered, [=](){
        QString display = tr("DXCC: %1/100\nWAC (ARRL): %2/6\nWAC (Non-ARRL): %3/6\nCQ: %4/70\nWAS: %5/50");
        GLogConcurrent::makeFuture([=](){
            auto res = model->diffEntNameCountForAward();
            return display
                .arg(res.DXCC)
                .arg(res.WAC_ARRL)
                .arg(res.WAC_NOTARRL)
                .arg(res.CQZ)
                .arg(res.WAS);
        }).then([=](QString result){
            emit information(tr("Award"), result, QMessageBox::StandardButton::Ok);
        });
    });

    connect(ui->actionUpdate_FCC_database, &QAction::triggered, this, &GLogApplication::updateFccDatabase);
}

GLogApplication::~GLogApplication()
{
    delete ui;
}

void GLogApplication::resizeTableView()
{
    tableview->setVisible(false);
    tableview->resizeColumnsToContents();
    tableview->resizeRowsToContents();
    tableview->setVisible(true);
}

void GLogApplication::openFile(const QString &filename)
{
    //tableview->setVisible(false);
    emit openFileActionSignal(filename);
}

void GLogApplication::openFileAction() 
{
    openFile(QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("Adif Files (*.adi *.adif);;All Files (*.*)")));
}

void GLogApplication::mergeFile(const QString &filename, bool remove)
{
    //tableview->setVisible(false);
    emit mergeFileActionSignal(filename, remove);
}

void GLogApplication::saveAsFile(const QString &filename)
{
    emit saveAsActionSignal(filename);
}

void GLogApplication::extractZip(const QString& zipPath, const QString& extractDir, QNetworkAccessManager& manager)
{
    QDir().mkpath(extractDir);
    QProcess proc;
#ifdef Q_OS_WIN
    proc.start("tar", { "-xf", zipPath, "-C", extractDir });
    if (!proc.waitForFinished() || proc.exitCode() != 0) {
        //throw std::runtime_error(tr("Extraction failed").toStdString());
        QString _7zipbin = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/7zip";
        QDir().mkpath(_7zipbin);
        QString _7zrExePath = _7zipbin + "/7zr.exe";
        QString _7zaExePath = _7zipbin + "/7za.exe";

        auto downloadProgress = [=, &manager](QString url, QString filename) {
            QFile file(filename);
            if (!file.exists()) {
                auto req = QNetworkRequest(QUrl(url));
                GLogNetwork::setGeneralHeader(req);
                auto rep = manager.get(req);
                QEventLoop loop;
                QObject::connect(rep, &QNetworkReply::finished, &loop, &QEventLoop::quit);
                loop.exec();
                if (rep->error() != QNetworkReply::NoError) {
                    rep->deleteLater();
                    throw std::runtime_error(tr("Extraction failed").toStdString());
                }
                if (!file.open(QIODevice::WriteOnly)) {
                    rep->deleteLater();
                    throw std::runtime_error(tr("Extraction failed").toStdString());
                }
                file.write(rep->readAll());
                file.close();
                rep->deleteLater();
            }
        };
        {
            QFile _7za_file(_7zaExePath);
            if (!_7za_file.exists()) {
                downloadProgress("https://www.7-zip.org/a/7zr.exe", _7zrExePath);
                QString _7za7zPath = QDir::tempPath() + "/7za.7z";
                downloadProgress("https://www.7-zip.org/a/7z2600-extra.7z", _7za7zPath);
                QProcess _7zr_proc;
                _7zr_proc.start(_7zrExePath, { "x", QDir::toNativeSeparators(_7za7zPath), "-o" + QDir::toNativeSeparators(_7zipbin), "7za.exe", "-y" });
                if (!_7zr_proc.waitForFinished() || _7zr_proc.exitCode() != 0) {
                    throw std::runtime_error(tr("Extraction failed").toStdString());
                }
            }
            QProcess _7za_proc;
            _7za_proc.start(_7zaExePath, { "x", QDir::toNativeSeparators(zipPath), "-o" + QDir::toNativeSeparators(extractDir), "-y" });
            if (!_7za_proc.waitForFinished() || _7za_proc.exitCode() != 0) {
                throw std::runtime_error(tr("Extraction failed").toStdString());
            }
        }
    }
#else
    proc.start("unzip", { "-o", zipPath, "-d", extractDir });
    if (!proc.waitForFinished() || proc.exitCode() != 0) {
        throw std::runtime_error(tr("Extraction failed").toStdString());
    }
#endif
}

void GLogApplication::initCtyDB(bool show_warning)
{
    qDebug() << "Checking DB initialization. Object Address:" << this
        << " | Thread:" << QThread::currentThread();

    auto ctydb = CtyDB::instance();
    configureCtyDialog->disableCtyConfigure();
    GLogConcurrent::makeFuture([=](){
        QEventLoop loop;
        QNetworkAccessManager manager;
        QNetworkRequest req(QUrl(QString::fromLatin1(CtyDB::DEFAULT_ONLINE_DATA)));
        GLogNetwork::setGeneralHeader(req);
        auto rep = manager.get(req);
        QObject::connect(rep, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        if (rep->error() == QNetworkReply::NoError) {
            auto cty = QString(rep->readAll());
            ctydb->loadDBString(cty, CtyDB::DEFAULT_ONLINE_DATA);
        } else {
            if (show_warning) emit warning(tr("Warning"), tr("Can not fetch cty.dat online. Using build-in one.\nYou should consider to check your network, or add a cty.dat manually.\n") + rep->errorString(), QMessageBox::StandardButton::Ok);
            ctydb->loadLocalDB();
        }
        rep->deleteLater();
    }).then(this, [=](){
        configureCtyDialog->enableCtyConfigure();
        emit initCtyDBDone();
    });
}

CtyDB *GLogApplication::getCtyDBInstance() const
{
    return CtyDB::instance();
}

void GLogApplication::mergeFileAction() 
{
    mergeFile(QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("Adif Files (*.adi *.adif);;All Files (*.*)")));
}


void GLogApplication::modelUpdated()
{
    //resizeTableView();
    //tableview->setVisible(true);
}

void GLogApplication::saveAsAction()
{
    if (model->rowCount() == 0) {
        return;
    }
    saveAsFile(QFileDialog::getSaveFileName(this, tr("Save As"), "", tr("Adif Files (*.adi *.adif);;Csv Files (*.csv)")));
}

void GLogApplication::saveDone()
{
    QMessageBox::information(this, tr("Save"), tr("Done"), QMessageBox::StandardButton::Ok);
}

void GLogApplication::setCilpboard(QMimeData *mimeData)
{
    QApplication::clipboard()->setMimeData(mimeData);
    QMessageBox::information(this, tr("Copy"), tr("Done"), QMessageBox::StandardButton::Ok);
}

void GLogApplication::updateFccDatabase()
{
    QString dbPath = FccDB::dbPath();
    auto button1 = QMessageBox::question(this, tr("FCC Database"), tr("This action will download complete FCC database (l_amat.zip) and make a simple one for WAS counting.\nDatabase will be save at %1.\nContinue?").arg(dbPath), QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No);
    if (button1 == QMessageBox::StandardButton::No) {
        return;
    }
    bool existsDB = QFile::exists(dbPath);
    QString oldDBPath = dbPath + "." + QDateTime::currentDateTime().toString("yyyyMMddHHmmss") + ".bak";
    if (existsDB) {
        auto oldDB = QFile(dbPath);
        if (!oldDB.rename(oldDBPath)){
            emit warning(tr("FCC Database"), tr("Cannot remove old database file"), QMessageBox::StandardButton::Ok);
            return;
        }
    }
    QString zipPath = QDir::tempPath() + "/l_amat.zip";
    QFileInfo zipInfo(zipPath);
    bool download = true;
    if (zipInfo.exists()) {
        auto button2 = QMessageBox::question(this, tr("FCC Database"), tr("Found %1.\nSkip download?").arg(zipPath), QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No);
        if (button2 == QMessageBox::StandardButton::Yes) {
            download = false;
        }
    }
    QString extractDir = QDir::tempPath() + "/fcc_extract";
    GLogConcurrent::makeFuture([=](QPromise<void> & promise){
        emit disableAction(ui->actionUpdate_FCC_database);
        emit disableAction(ui->actionAward);
        QNetworkAccessManager manager;
        if (download) {
            emit showMessage(tr("Downloading l_amat.zip"));
            QUrl url("https://data.fcc.gov/download/pub/uls/complete/l_amat.zip");
            auto req = QNetworkRequest(url);
            GLogNetwork::setGeneralHeader(req);
            QNetworkReply* reply = manager.get(req);
            GLogNetwork::watchGetProcess(reply, [=](QString msg){
                emit showMessage(tr("Downloading l_amat.zip: %1").arg(msg));
            });
            QEventLoop loop;
            QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();
            if (reply->error() != QNetworkReply::NoError) {
                auto err = reply->errorString().toStdString();
                reply->deleteLater();
                throw std::runtime_error(err);
            }
            QFile file(zipPath);
            if (file.open(QIODevice::WriteOnly)) {
                emit showMessage(tr("Saving l_amat.zip"));
                file.write(reply->readAll());
                file.close();
            }
            reply->deleteLater();
        }
        emit showMessage(tr("Extracting l_amat.zip"));

        extractZip(zipPath, extractDir, manager);

        QString hdPath = extractDir + "/HD.dat";
        QString enPath = extractDir + "/EN.dat";

        QFileInfo dbInfo(dbPath);
        QDir().mkpath(dbInfo.absoluteDir().absolutePath());

        emit showMessage(tr("Updating database"));

        QString connectionName = QString("fcc_build_%1").arg(quintptr(QThread::currentThreadId()));
        {
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
            db.setDatabaseName(dbPath);
            if (!db.open()) throw std::runtime_error(tr("Database open failed").toStdString());

            QHash<long long, QString> usiToState;
            QFile enFile(enPath);
            int enCount = 0;
            if (enFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                while (!enFile.atEnd()) {
                    if (promise.isCanceled()) {
                        return;
                    }
                    if (enCount % 10000 == 0) emit showMessage(tr("Processing EN.dat: %1").arg(enCount));
                    QByteArray line = enFile.readLine();
                    if (!line.startsWith("EN|")) continue;

                    auto fields = line.split('|');

                    if (fields.size() < 18) continue;

                    long long usi = fields[1].toLongLong();

                    QString stateCandidate = QString::fromLatin1(fields[17]).trimmed();

                    if (stateCandidate.length() == 2 && stateCandidate == stateCandidate.toUpper()) {
                        if (usi > 0) {
                            usiToState.insert(usi, stateCandidate);
                            ++enCount;
                        }
                    }
                    else {
                        for (int i : {12, 16, 17, 18, 19, 20}) {
                            if (i < fields.size()) {
                                QString s = QString::fromLatin1(fields[i]).trimmed();
                                if (s.length() == 2 && s == s.toUpper()) {
                                    usiToState.insert(usi, s);
                                    ++enCount;
                                    break;
                                }
                            }
                        }
                    }
                }
                enFile.close();
            }

            QSqlQuery query(db);
            query.exec("PRAGMA synchronous = OFF;"); 
            query.exec("PRAGMA journal_mode = MEMORY;");
            query.exec("CREATE TABLE licenses (callsign TEXT PRIMARY KEY, state TEXT)");

            QFile hdFile(hdPath);
            if (hdFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                db.transaction(); 
                query.prepare("INSERT OR REPLACE INTO licenses (callsign, state) VALUES (?, ?)");
                
                int hdCount = 0;
                while (!hdFile.atEnd()) {
                    if (promise.isCanceled()) {
                        db.rollback();
                        return;
                    }
                    if (hdCount % 10000 == 0) emit showMessage(tr("Processing HD.dat: %1").arg(hdCount));
                    QByteArray line = hdFile.readLine();
                    auto fields = line.split('|');
                    // Index 1: USI, Index 4: Callsign
                    if (fields.size() > 5) {
                        long long usi = fields[1].toLongLong();
                        if (usiToState.contains(usi) && !fields[4].isEmpty() && fields[5] == "A") {
                            query.addBindValue(QString::fromLatin1(fields[4])); // Callsign
                            query.addBindValue(usiToState[usi]);               // State
                            query.exec();
                            hdCount++;
                        }
                    }

                    if (hdCount && hdCount % 10000 == 0) {
                        db.commit();
                        db.transaction();
                    }
                }
                db.commit();
                hdFile.close();
                
                query.exec("CREATE INDEX idx_call ON licenses(callsign)");
            }
            db.close();
        }
        QSqlDatabase::removeDatabase(connectionName);

    }).then([=](){
        if (existsDB) {
            QFile::remove(oldDBPath);
        }
        QDir(extractDir).removeRecursively();
        emit information(tr("FCC Database"), tr("FCC database updated."), QMessageBox::StandardButton::Ok);
        emit showMessage(tr("Done"), 5000);
        emit enableAction(ui->actionUpdate_FCC_database);
        emit enableAction(ui->actionAward);
    }).onFailed([=](const std::exception& e){
        if (existsDB) {
            auto oldDB = QFile(oldDBPath);
            oldDB.rename(dbPath);
        }
        QDir(extractDir).removeRecursively();
        emit warning(tr("FCC Database"), tr("FCC database failed:%1").arg(e.what()), QMessageBox::StandardButton::Ok);
        emit showMessage(tr("Failed"), 5000);
        emit enableAction(ui->actionUpdate_FCC_database);
        emit enableAction(ui->actionAward);
    }).onCanceled([=](){
        if (existsDB) {
            auto oldDB = QFile(oldDBPath);
            oldDB.rename(dbPath);
        }
        QDir(extractDir).removeRecursively();
        emit information(tr("FCC Database"), tr("Update canceled."), QMessageBox::StandardButton::Ok);
        emit showMessage(tr("Canceled"), 5000);
        emit enableAction(ui->actionUpdate_FCC_database);
        emit enableAction(ui->actionAward);
    });
}
