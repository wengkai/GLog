#ifndef CONFIGURECTYDIALOG_H
#define CONFIGURECTYDIALOG_H

#include <QDialog>
#include "app_export.h"
namespace Ui {
class ConfigureCtyDialogClass;
};

class APP_EXPORT ConfigureCtyDialog : public QDialog {
    Q_OBJECT

    struct LoadContext {
        QString targetUrl; // db_hint
        QString message;   // load_hint
        bool isRollingBack = false;
    };

    void processLoadResult(const LoadContext &ctx, const QPair<bool, QString> &res);
    void handleLocalLoad(const LoadContext &ctx);
    void handleRemoteLoad(const LoadContext &ctx);
    void handleNetworkFailure(const LoadContext &ctx, const QString &error);

  public:
    ConfigureCtyDialog(QWidget *parent = nullptr);
    ~ConfigureCtyDialog();
    static QString successMsg();

  public slots:
    void setDBhint(const QString &db_hint);
    void disableCtyConfigure();
    int exec() override;
    void enableCtyConfigure();
    void applyLoadDB(const LoadContext &ctx);
    void onLoadFinished(const QString &msg);

  protected slots:

  signals:
    void startLoadDB(const LoadContext &ctx);
    void endLoadDB(QString load_hint);

  private:
    Ui::ConfigureCtyDialogClass *ui;
    QString db_hint;
    QPushButton *m_ok = nullptr;
    int disable_count = 0;
};

#endif