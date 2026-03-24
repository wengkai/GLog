#include "AddQSODialog.h"
#include "ui_AddQSODialog.h"
#include <QTime>
#include <QDate>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QMessageBox>

AddQSODialog::AddQSODialog(AdifModel * model, QWidget *parent) : m_model(model), QDialog(parent), ui(new Ui::AddQSODialogClass())
{
    ui->setupUi(this);
    for (auto & mode_pair : mode_map) {
        if (mode_pair.second.mode_submode.empty()) {
            ui->modeComboBox->addItem(QString::fromStdString(mode_pair.first) + (mode_pair.second.import_only ? "(import-only)" : ""));
            continue;
        }
        for (auto & sub_mode : mode_pair.second.mode_submode) {
            ui->modeComboBox->addItem(QString::fromStdString(sub_mode) + "="  + QString::fromStdString(mode_pair.first));
        }
    }
}

AddQSODialog::~AddQSODialog()
{
    delete ui;
}

void AddQSODialog::accept()
{
    if (ui->callSignLineEdit->text().isEmpty()
        || ui->modeComboBox->currentText().isEmpty()
        || ui->freqDoubleSpinBox->text().isEmpty()
        || ui->rstLineEdit->text().isEmpty()
        || ui->rstRcvdLineEdit->text().isEmpty()
    ) {
        QMessageBox::information(this, tr("Add QSO"), tr("Some empty field(s)."), QMessageBox::StandardButton::Ok);
        return;
    }
    static auto call_regex = QRegularExpression("^([A-Z0-9]+|[A-Z0-9]+\\/[A-Z0-9]+)$");
    if (!call_regex.match(ui->callSignLineEdit->text()).hasMatch()) {
        QMessageBox::information(this, tr("Add QSO"), tr("Invaild Call."), QMessageBox::StandardButton::Ok);
        return;
    }
    static auto rst_regex = QRegularExpression("^[+-]?\\d{1,3}$");
    if (!rst_regex.match(ui->rstLineEdit->text()).hasMatch()) {
        QMessageBox::information(this, tr("Add QSO"), tr("Invaild Rst."), QMessageBox::StandardButton::Ok);
        return;
    }
    if (!rst_regex.match(ui->rstRcvdLineEdit->text()).hasMatch()) {
        QMessageBox::information(this, tr("Add QSO"), tr("Invaild Rst Rcvd."), QMessageBox::StandardButton::Ok);
        return;
    }
    // {"qso_date", "time_on", "call", "freq", "mode", "rst_rcvd", "rst_sent"}
    GRecord record;
    record["qso_date"] = ui->dateEdit->date().toString("yyyyMMdd").toStdString();
    record["time_on"] = ui->timeEdit->time().toString("HHmmss").toStdString();
    record["call"] = ui->callSignLineEdit->text().toStdString();
    record["freq"] = ui->freqDoubleSpinBox->text().toStdString();
    auto mode_texts = ui->modeComboBox->currentText().split("=");
    if (mode_texts.size() == 1) {
        record["mode"] = mode_texts[0].toStdString();
    } else {
        record["submode"] = mode_texts[0].toStdString();
        record["mode"] = mode_texts[1].toStdString();
    }
    record["rst_sent"] = ui->rstLineEdit->text().toStdString();
    record["rst_rcvd"] = ui->rstRcvdLineEdit->text().toStdString();
    m_model->addRecord(record);
    //QDialog::accept();
}
