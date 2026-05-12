#ifndef ADDQSODIALOG_H
#define ADDQSODIALOG_H

#include <QDialog>
#include "adifdb.h"
#include "app_export.h"
namespace Ui {
class AddQSODialogClass;
};

class GLOGKIT_API AddQSODialog : public QDialog {
    Q_OBJECT

  public:
    AddQSODialog(AdifModel *model, QWidget *parent = nullptr);
    ~AddQSODialog();

  protected slots:
    void accept() override;

  signals:
    void userInformation(const QString &title, const QString &text);
    void userWarning(const QString &title, const QString &text);

  private:
    Ui::AddQSODialogClass *ui = nullptr;
    AdifModel *m_model = nullptr;
};

#endif