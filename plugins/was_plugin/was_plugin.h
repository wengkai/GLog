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

class WASPlugin {
  public:
    WASPlugin();
    ~WASPlugin();

    const char *pluginName() const noexcept;
    bool install() noexcept;
    bool uninstall() noexcept;
    bool beforeEvaluate() noexcept;
    bool afterEvaluate() noexcept;
    bool evaluate(const IGRecord *QSO, IGRecordGetValueByField callback) noexcept;
    void getResult(char *result_buf, uint64_t *result_len, uint64_t max_result_len) const noexcept;
    void getLastError(char *result_buf, uint64_t *result_len,
                      uint64_t max_result_len) const noexcept;

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