#ifndef CONFIGURECTYDIALOG_H
#define CONFIGURECTYDIALOG_H

#include <QDialog>
#include "app_export.h"
namespace Ui {
class ConfigureCtyDialogClass;
};

class APP_EXPORT ConfigureCtyDialog : public QDialog {
    Q_OBJECT

  public:
    ConfigureCtyDialog(QWidget *parent = nullptr);
    ~ConfigureCtyDialog();
    static QString successMsg();

  public slots:
    void setDBhint(const QString &db_hint);
    void disableCtyConfigure();
    int exec() override;
    void enableCtyConfigure();
    void applyLoadDB(const QString &db_hint, const QString &load_hint, bool rollBack = false);
    void onLoadFinished(const QString &msg);

  protected slots:

  signals:
    void startLoadDB(QString db_hint, QString load_hint, bool rollBack = false);
    void endLoadDB(QString load_hint);

  private:
    Ui::ConfigureCtyDialogClass *ui;
    QString db_hint;
    QPushButton *m_ok = nullptr;
    int disable_count = 0;
};

#endif