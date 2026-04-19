#ifndef WAS_PLUGIN_H
#define WAS_PLUGIN_H

#include <set>
#include <string>
#include <vector>

#include <GLogKit/award_plugin.h>

#include <QtCore/QCache>
#include <QtCore/QHash>
#include <QtCore/QStandardPaths>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

class WASPlugin : public AwardPlugin {
  public:
    WASPlugin();
    ~WASPlugin() override;

    const char *pluginName() const noexcept override;
    bool install() noexcept override;
    bool uninstall() noexcept override;
    bool beforeEvaluate() noexcept override;
    bool afterEvaluate() noexcept override;
    bool evaluate(const char *QSO, unsigned long long len) noexcept override;
    const char *getResult() const noexcept override;
    const char *getLastError() const noexcept override;
    void deleteString(const char *str) noexcept override;

  private:
    bool updateFccDatabase();
    bool openDatabase();
    void closeDatabase();
    QString queryStateByCallsign(const QString &callsign);
    QString parseCallsignFromADIF(const QString &adif);
    QString parseStateFromADIF(const QString &adif);
    bool isValidUSState(const QString &state) const;

    QString connName;
    QCache<QString, QString> m_cache{5000};
    QSqlQuery m_query;
    QSet<QString> m_workedStates;
    mutable QString m_lastError;
    mutable QString m_cachedResult;
    bool m_installed;
};

#endif