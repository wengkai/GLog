#include "AddQSODialog.h"
#include <QDate>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QTime>
#include "AdifDataTypes.h"
#include "ui_AddQSODialog.h"

AddQSODialog::AddQSODialog(AdifModel *model, QWidget *parent)
    : m_model(model), QDialog(parent), ui(new Ui::AddQSODialogClass()) {
    ui->setupUi(this);
    const auto &MODE_MAP = AdifModeFactory::getModeMap();
    for (const auto &mode_pair : MODE_MAP) {
        if (mode_pair.second.submodes.empty()) {
            ui->modeComboBox->addItem(QString::fromStdString(mode_pair.first) +
                                      (mode_pair.second.import_only ? "(import-only)" : ""));
            continue;
        }
        for (const auto &sub_mode : mode_pair.second.submodes) {
            ui->modeComboBox->addItem(QString::fromStdString(sub_mode) + "=" +
                                      QString::fromStdString(mode_pair.first));
        }
    }
}

AddQSODialog::~AddQSODialog() { delete ui; }

void AddQSODialog::accept() {
    if (ui->callSignLineEdit->text().isEmpty() || ui->modeComboBox->currentText().isEmpty() ||
        ui->freqDoubleSpinBox->text().isEmpty() || ui->rstLineEdit->text().isEmpty() ||
        ui->rstRcvdLineEdit->text().isEmpty()) {
        emit userInformation(tr("Add QSO"), tr("Some empty field(s)."));
        return;
    }
    // 1. Unified Validation
    const QString call = ui->callSignLineEdit->text().trimmed();
    const QString rstS = ui->rstLineEdit->text().trimmed();
    const QString rstR = ui->rstRcvdLineEdit->text().trimmed();

    // 2. Regex Checks (Static to avoid re-compilation)
    static const QRegularExpression call_regex(R"(^([A-Z0-9]+|[A-Z0-9]+\/[A-Z0-9]+)$)",
                                               QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression rst_regex(R"(^[+-]?\d{1,3}$)");

    if (!call_regex.match(call).hasMatch()) {
        ui->callSignLineEdit->setFocus();
        emit userWarning(tr("Add QSO"), tr("Invalid Call-sign format."));
        return;
    }

    // 3. Data Population
    GRecord record;

    // Dates/Times: ADIF spec requires YYYYMMDD and HHMMSS
    record.addOrSetPair("qso_date", ui->dateEdit->date().toString("yyyyMMdd").toStdString());
    record.addOrSetPair("time_on", ui->timeEdit->time().toString("HHmmss").toStdString());
    record.addOrSetPair("call", call.toUpper().toStdString());

    // Frequency: Use cleanText() or value() to avoid locale symbols like '$' or unit suffixes
    record.addOrSetPair("freq",
                        QString::number(ui->freqDoubleSpinBox->value(), 'f', 3).toStdString());

    // Mode handling: Note how we handle the split
    QStringList modeParts = ui->modeComboBox->currentText().split('=');
    if (modeParts.size() >= 2) {
        // Set Mode FIRST, then Submode to trigger linkPeers correctly
        record.addOrSetPair("mode", modeParts[1].trimmed().toStdString());
        record.addOrSetPair("submode", modeParts[0].trimmed().toStdString());
    } else {
        record.addOrSetPair("mode", modeParts[0].trimmed().toStdString());
    }

    record.addOrSetPair("rst_sent", rstS.toStdString());
    record.addOrSetPair("rst_rcvd", rstR.toStdString());

    // 4. Model Update
    // Pass by move if insertRecords supports it to avoid the persistence clone overhead
    m_model->insertRecords(-1, {std::move(record)});

    // Close the dialog properly
    QDialog::accept();
}
