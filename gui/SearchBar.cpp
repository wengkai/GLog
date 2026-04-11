#include "SearchBar.h"
#include <QMessageBox>
#include "ui_SearchBar.h"

SearchBar::SearchBar(QWidget *parent) : QDialog(parent), ui(new Ui::SearchBarClass()) {
    ui->setupUi(this);
    connect(ui->findNextButton, &QPushButton::clicked, this, [=]() {
        disableButtons();
        emit findNext(ui->lineEdit->text().toLower(), ui->plainTextEdit->toPlainText(),
                      ui->regexModeCheckBox->isChecked());
    });
    connect(ui->selectAllButton, &QPushButton::clicked, this, [=]() {
        disableButtons();
        emit selectAll(ui->lineEdit->text().toLower(), ui->plainTextEdit->toPlainText(),
                       ui->regexModeCheckBox->isChecked());
    });
    connect(ui->clearAllButton, &QPushButton::clicked, this, [=]() {
        disableButtons();
        emit deselectAll(ui->lineEdit->text().toLower(), ui->plainTextEdit->toPlainText(),
                         ui->regexModeCheckBox->isChecked());
    });
}

SearchBar::~SearchBar() { delete ui; }

void SearchBar::disableButtons() {
    ui->findNextButton->setDisabled(true);
    ui->selectAllButton->setDisabled(true);
    ui->clearAllButton->setDisabled(true);
}

void SearchBar::enableButtons() {
    ui->findNextButton->setEnabled(true);
    ui->selectAllButton->setEnabled(true);
    ui->clearAllButton->setEnabled(true);
}