#ifndef ADDQSODIALOG_H
#define ADDQSODIALOG_H

#include "adifdb.h"
#include <QDialog>
namespace Ui{ class AddQSODialogClass; };

class AddQSODialog : public QDialog
{
    Q_OBJECT

public:
    AddQSODialog(AdifModel * model, QWidget *parent = nullptr);
    ~AddQSODialog();

protected slots:
    void accept() override;

private:
    Ui::AddQSODialogClass * ui = nullptr;
    AdifModel * m_model = nullptr;

};

#endif