#ifndef ADDQSODIALOG_H
#define ADDQSODIALOG_H

#include "adifdb.h"
#include "ui_AddQSODialog.h"

class AddQSODialog : public QDialog
{
    Q_OBJECT

public:
    AddQSODialog(AdifModel * model, QWidget *parent = nullptr);
    ~AddQSODialog();

protected slots:
    void accept() override;

private:
    Ui::AddQSODialogClass ui;
    AdifModel * m_model = nullptr;

};

#endif