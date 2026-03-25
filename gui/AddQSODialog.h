#ifndef ADDQSODIALOG_H
#define ADDQSODIALOG_H

#include "app_export.h"
#include "adifdb.h"
#include <QDialog>
namespace Ui{ class AddQSODialogClass; };

class APP_EXPORT AddQSODialog : public QDialog
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