#include "ConfigureCtyDialog.h"
#include <QFileDialog>
#include <QUrl>
#include "Concurrent.h"
#include "GlobalNetwork.h"
#include "ctydb.h"
#include "ui_ConfigureCtyDialog.h"

void ConfigureCtyDialog::processLoadResult(const LoadContext &ctx,
                                           const QPair<bool, QString> &res) {
    auto *ctydb = CtyDB::instance();
    const QString &errorMsg = res.second;

    if (res.first) {
        QString finalMsg =
            ctx.isRollingBack ? ctx.message + tr("Using previous one.") : ctx.message;
        emit endLoadDB(finalMsg);
    } else {
        if (ctx.isRollingBack) {
            ctydb->loadLocalDB();
            emit endLoadDB(ctx.message + tr("Using build-in default one."));
        } else {
            emit startLoadDB({this->db_hint, errorMsg, true});
        }
    }
}

void ConfigureCtyDialog::handleLocalLoad(const LoadContext &ctx) {
    QString localPath = QUrl(ctx.targetUrl).toLocalFile();
    GLogConcurrent::makeFuture([this, ctx, localPath]() {
        auto res = CtyDB::instance()->loadLocalDB(localPath);
        processLoadResult(ctx, res);
    });
}

void ConfigureCtyDialog::handleRemoteLoad(const LoadContext &ctx) {
    GLogNetwork::get(ctx.targetUrl, [this, ctx](QNetworkReply *rep) {
        QString content = QString(rep->readAll());
        auto networkError = rep->error();
        auto errorStr = rep->errorString();

        GLogConcurrent::makeFuture([this, ctx, content, networkError, errorStr]() {
            if (networkError != QNetworkReply::NoError) {
                handleNetworkFailure(ctx, errorStr);
                return;
            }
            auto res = CtyDB::instance()->loadDBString(content, ctx.targetUrl);
            processLoadResult(ctx, res);
        });
    });
}

void ConfigureCtyDialog::handleNetworkFailure(const LoadContext &ctx, const QString &error) {
    if (ctx.isRollingBack) {
        CtyDB::instance()->loadLocalDB();
        emit endLoadDB(ctx.message + tr("Using build-in default one. Network failed:") + error);
    } else {
        emit endLoadDB(tr("No change applied. Network failed:") + error);
    }
}

ConfigureCtyDialog::ConfigureCtyDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::ConfigureCtyDialogClass()) {
    ui->setupUi(this);
    m_ok = ui->buttonBox->button(QDialogButtonBox::StandardButton::Ok);
    ui->DBhint->setText(tr("Cty.dat:"));
    db_hint = tr("None");
    disconnect(ui->buttonBox, &QDialogButtonBox::accepted, this, qOverload<>(&QDialog::accept));
    connect(ui->BrowseButton, &QPushButton::clicked, [=]() {
        auto localFile = QFileDialog::getOpenFileName(
            this, tr("Open File"), "", tr("Cty Data Files (*.dat);;All Files (*.*)"));
        if (!localFile.isEmpty()) {
            ui->lineEdit->setText(QUrl::fromLocalFile(localFile).toString());
        }
    });
    connect(this, &ConfigureCtyDialog::startLoadDB, this, &ConfigureCtyDialog::applyLoadDB,
            Qt::QueuedConnection);
    connect(m_ok, &QPushButton::clicked, [=]() {
        disableCtyConfigure();
        emit startLoadDB({ui->lineEdit->text(), successMsg(), false});
    });
    connect(this, &ConfigureCtyDialog::endLoadDB, this, &ConfigureCtyDialog::onLoadFinished,
            Qt::QueuedConnection);
}

void ConfigureCtyDialog::applyLoadDB(const LoadContext &ctx) {
    if (!ctx.isRollingBack && ctx.targetUrl == this->db_hint) {
        enableCtyConfigure();
        accept();
        return;
    }

    QUrl url(ctx.targetUrl);
    if (!url.isValid()) {
        enableCtyConfigure();
        return;
    }

    if (url.isLocalFile()) {
        handleLocalLoad(ctx);
    } else {
        handleRemoteLoad(ctx);
    }
}

void ConfigureCtyDialog::onLoadFinished(const QString &msg) {
    emit loadFinishedInformation(msg);
    enableCtyConfigure();
    if (msg == successMsg()) {
        accept();
    }
}

ConfigureCtyDialog::~ConfigureCtyDialog() { delete ui; }

auto ConfigureCtyDialog::successMsg() -> QString { return tr("Success."); }

void ConfigureCtyDialog::disableCtyConfigure() {
    ++disable_count;
    m_ok->setEnabled(false);
}

void ConfigureCtyDialog::enableCtyConfigure() {
    Q_ASSERT(disable_count > 0);
    --disable_count;
    if (disable_count == 0) {
        m_ok->setEnabled(true);
    }
}

auto ConfigureCtyDialog::exec() -> int {
    ui->lineEdit->setText(db_hint);
    return QDialog::exec();
}

void ConfigureCtyDialog::setDBhint(const QString &db_hint) {
    this->db_hint = db_hint;
    ui->lineEdit->setText(db_hint);
}
