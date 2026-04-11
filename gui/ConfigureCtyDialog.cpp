#include "ConfigureCtyDialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QUrl>
#include "Concurrent.h"
#include "GlobalNetwork.h"
#include "ctydb.h"
#include "ui_ConfigureCtyDialog.h"

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
        emit startLoadDB(ui->lineEdit->text(), successMsg(), false);
    });
    connect(this, &ConfigureCtyDialog::endLoadDB, this, &ConfigureCtyDialog::onLoadFinished,
            Qt::QueuedConnection);
}

void ConfigureCtyDialog::applyLoadDB(const QString &db_hint, const QString &load_hint,
                                     bool rollBack) {
    if (!rollBack && db_hint == this->db_hint) {
        enableCtyConfigure();
        accept();
        return;
    }
    auto pre_db_hint = this->db_hint;
    auto url = QUrl(db_hint);
    auto *ctydb = CtyDB::instance();
    if (!url.isValid()) {
        enableCtyConfigure();
        return;
    }
    if (url.isLocalFile()) {
        auto localFile = url.toLocalFile();
        auto future = GLogConcurrent::makeFuture([=]() {
            auto ret = ctydb->loadLocalDB(localFile);
            if (ret.first) {
                if (rollBack) {
                    emit endLoadDB(load_hint + tr("Using previous one."));
                    return;
                }
                emit endLoadDB(load_hint);
            } else {
                if (rollBack) {
                    emit endLoadDB(load_hint + tr("Using build-in default one."));
                    ctydb->loadLocalDB();
                    return;
                }
                emit startLoadDB(pre_db_hint, ret.second, true);
            }
        });
        return;
    }
    GLogNetwork::get(db_hint, [=](QNetworkReply *rep) {
        auto cty = QString(rep->readAll());
        auto rep_error = rep->error();
        auto rep_errorString = rep->errorString();
        auto future = GLogConcurrent::makeFuture([=]() {
            if (rep_error == QNetworkReply::NoError) {
                auto ret = ctydb->loadDBString(cty, db_hint);
                if (ret.first) {
                    if (rollBack) {
                        emit endLoadDB(load_hint + tr("Using previous one."));
                        return;
                    }
                    emit endLoadDB(load_hint);
                } else {
                    if (rollBack) {
                        emit endLoadDB(load_hint + tr("Using build-in default one."));
                        ctydb->loadLocalDB();
                        return;
                    }
                    emit startLoadDB(pre_db_hint, ret.second, true);
                }
            } else {
                if (rollBack) {
                    emit endLoadDB(load_hint + tr("Using build-in default one. Network failed:") +
                                   rep_errorString);
                    ctydb->loadLocalDB();
                    return;
                }
                emit endLoadDB(tr("No change is applied. Network failed:") + rep_errorString);
            }
        });
    });
}

void ConfigureCtyDialog::onLoadFinished(const QString &msg) {
    QMessageBox::information(this, tr("Load"), msg);
    enableCtyConfigure();
    if (msg == successMsg()) {
        accept();
    }
}

ConfigureCtyDialog::~ConfigureCtyDialog() { delete ui; }

QString ConfigureCtyDialog::successMsg() { return tr("Success."); }

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

int ConfigureCtyDialog::exec() {
    ui->lineEdit->setText(db_hint);
    return QDialog::exec();
}

void ConfigureCtyDialog::setDBhint(const QString &db_hint) {
    this->db_hint = db_hint;
    ui->lineEdit->setText(db_hint);
}
