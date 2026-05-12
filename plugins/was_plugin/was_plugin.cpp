#include "was_plugin.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QTextStream>
#include <QtCore/QThread>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <stdexcept>

WASPlugin instance;

static const QSet<QString> US_STATES = {"AL", "AK", "AZ", "AR", "CA", "CO", "CT", "DE", "FL", "GA",
                                        "HI", "ID", "IL", "IN", "IA", "KS", "KY", "LA", "ME", "MD",
                                        "MA", "MI", "MN", "MS", "MO", "MT", "NE", "NV", "NH", "NJ",
                                        "NM", "NY", "NC", "ND", "OH", "OK", "OR", "PA", "RI", "SC",
                                        "SD", "TN", "TX", "UT", "VT", "VA", "WA", "WV", "WI", "WY"};

WASPlugin::WASPlugin()
    : connName("was_plugin_conn_" + QString::number(reinterpret_cast<quintptr>(this))) {
    m_cache.setMaxCost(5000);
}

WASPlugin::~WASPlugin() { closeDatabase(); }

auto WASPlugin::pluginName() const noexcept -> const char * { return "WAS Plugin (FCC Database)"; }

auto WASPlugin::install() noexcept -> bool {
    try {
        return updateFccDatabase();
    } catch (...) {
        m_lastError = "Unknown exception during install";
        return false;
    }
}

auto WASPlugin::uninstall() noexcept -> bool {
    try {
        closeDatabase();
        QString dbPath =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/fcc_amat.sqlite";
        QFile::remove(dbPath);
        return true;
    } catch (...) {
        m_lastError = "Unknown exception during uninstall";
        return false;
    }
}

auto WASPlugin::beforeEvaluate() noexcept -> bool {
    try {
        m_workedStates.clear();
        m_cachedResult.clear();
        if (!openDatabase()) {
            m_lastError =
                "Failed to open database: " + QSqlDatabase::database(connName).lastError().text();
            return false;
        }
        return true;
    } catch (...) {
        m_lastError = "Unknown exception in beforeEvaluate";
        return false;
    }
}

auto WASPlugin::afterEvaluate() noexcept -> bool {
    try {
        QJsonObject result;
        result["total_states"] = static_cast<int>(m_workedStates.size());
        result["total_possible"] = 50;

        QJsonArray workedArray;
        for (const auto &state : m_workedStates) {
            workedArray.append(state);
        }
        result["worked_states"] = workedArray;

        QSet<QString> missing = US_STATES;
        missing.subtract(m_workedStates);
        QJsonArray missingArray;
        for (const auto &state : missing) {
            missingArray.append(state);
        }
        result["missing_states"] = missingArray;

        QJsonDocument doc(result);
        m_cachedResult = doc.toJson(QJsonDocument::Compact);

        closeDatabase();
        return true;
    } catch (...) {
        m_lastError = "Unknown exception in afterEvaluate";
        return false;
    }
}

auto WASPlugin::evaluate(const IGRecord *QSO, IGRecordGetValueByField callback) noexcept -> bool {
    try {
        if ((QSO == nullptr) || (callback == nullptr)) {
            return false;
        }
        auto getValueByField = [=](const char *field, uint64_t field_len, char *result_buf,
                                   uint64_t *result_len, uint64_t max_result_len) {
            return callback(QSO, field, field_len, result_buf, result_len, max_result_len);
        };

        char callBuf[64] = {0};
        uint64_t callLen = 0;
        auto res = getValueByField("call", 4, callBuf, &callLen, sizeof(callBuf) - 1);
        if (res != IGRecord::Result::NoError || callLen == 0) {
            return false;
        }
        callBuf[callLen] = '\0';
        QString callsign = QString::fromUtf8(callBuf).trimmed().toUpper();

        char stateBuf[8] = {0};
        uint64_t stateLen = 0;
        QString state;
        res = getValueByField("state", 5, stateBuf, &stateLen, sizeof(stateBuf) - 1);
        if (res == IGRecord::Result::NoError && stateLen > 0) {
            stateBuf[stateLen] = '\0';
            state = QString::fromUtf8(stateBuf).trimmed().toUpper();
        }

        if (state.isEmpty() && !callsign.isEmpty()) {
            state = queryStateByCallsign(callsign);
        }

        if (isValidUSState(state)) {
            m_workedStates.insert(state);
            return true;
        }

        return false;
    } catch (...) {
        m_lastError = "Exception during evaluate";
        return false;
    }
}

void WASPlugin::getResult(char *result_buf, uint64_t *result_len,
                          uint64_t max_result_len) const noexcept {
    try {
        if (m_cachedResult.isEmpty()) {
            if (result_len != nullptr) {
                *result_len = 0;
            }
            if ((result_buf != nullptr) && max_result_len > 0) {
                result_buf[0] = '\0';
            }
            return;
        }

        QByteArray utf8 = m_cachedResult.toUtf8();
        auto actual_len = static_cast<uint64_t>(utf8.size());

        if (result_len != nullptr) {
            *result_len = actual_len;
        }

        if ((result_buf != nullptr) && max_result_len > 0) {
            uint64_t copy_len = std::min(actual_len, max_result_len - 1);
            std::memcpy(result_buf, utf8.constData(), copy_len);
            result_buf[copy_len] = '\0';
        }
    } catch (...) {
        if (result_len != nullptr) {
            *result_len = 0;
        }
        if ((result_buf != nullptr) && max_result_len > 0) {
            result_buf[0] = '\0';
        }
    }
}

void WASPlugin::getLastError(char *result_buf, uint64_t *result_len,
                             uint64_t max_result_len) const noexcept {
    try {
        uint64_t full_len = 0;
        if (!m_lastError.isEmpty()) {
            QByteArray utf8 = m_lastError.toUtf8();
            full_len = static_cast<uint64_t>(utf8.size());
        }

        if (result_len != nullptr) {
            *result_len = full_len;
        }

        if ((result_buf == nullptr) || max_result_len == 0) {
            return;
        }

        if (full_len == 0) {
            result_buf[0] = '\0';
        } else {
            QByteArray utf8 = m_lastError.toUtf8();
            uint64_t copy_len = full_len;
            if (copy_len >= max_result_len) {
                copy_len = max_result_len - 1;
            }
            std::memcpy(result_buf, utf8.constData(), copy_len);
            result_buf[copy_len] = '\0';
        }
    } catch (...) {
        if (result_len != nullptr) {
            *result_len = 0;
        }
        if ((result_buf != nullptr) && max_result_len > 0) {
            result_buf[0] = '\0';
        }
    }
}

namespace {

void extractZipWas(const QString &zipPath, const QString &extractDir,
                   QNetworkAccessManager &manager) {
    QDir().mkpath(extractDir);
    QProcess proc;
#ifdef Q_OS_WIN
    proc.start(QStringLiteral("tar"),
               {QStringLiteral("-xf"), zipPath, QStringLiteral("-C"), extractDir});
    if (!proc.waitForFinished() || proc.exitCode() != 0) {
        QString _7zipbin = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
                           QStringLiteral("/7zip");
        QDir().mkpath(_7zipbin);
        QString _7zrExePath = _7zipbin + QStringLiteral("/7zr.exe");
        QString _7zaExePath = _7zipbin + QStringLiteral("/7za.exe");

        auto downloadProgress = [&](const QString &url, const QString &filename) {
            QFile file(filename);
            if (!file.exists()) {
                auto req = QNetworkRequest(QUrl(url));
                req.setHeader(QNetworkRequest::UserAgentHeader,
                              QStringLiteral("GLogKit-WAS-Plugin/1.0"));
                auto *rep = manager.get(req);
                QEventLoop loop;
                QObject::connect(rep, &QNetworkReply::finished, &loop, &QEventLoop::quit);
                loop.exec();
                if (rep->error() != QNetworkReply::NoError) {
                    rep->deleteLater();
                    throw std::runtime_error("Extraction failed");
                }
                if (!file.open(QIODevice::WriteOnly)) {
                    rep->deleteLater();
                    throw std::runtime_error("Extraction failed");
                }
                file.write(rep->readAll());
                file.close();
                rep->deleteLater();
            }
        };
        {
            QFile _7za_file(_7zaExePath);
            if (!_7za_file.exists()) {
                downloadProgress(QStringLiteral("https://www.7-zip.org/a/7zr.exe"), _7zrExePath);
                QString _7za7zPath = QDir::tempPath() + QStringLiteral("/7za.7z");
                downloadProgress(QStringLiteral("https://www.7-zip.org/a/7z2600-extra.7z"),
                                 _7za7zPath);
                QProcess _7zr_proc;
                _7zr_proc.start(_7zrExePath,
                                {QStringLiteral("x"), QDir::toNativeSeparators(_7za7zPath),
                                 QStringLiteral("-o") + QDir::toNativeSeparators(_7zipbin),
                                 QStringLiteral("7za.exe"), QStringLiteral("-y")});
                if (!_7zr_proc.waitForFinished() || _7zr_proc.exitCode() != 0) {
                    throw std::runtime_error("Extraction failed");
                }
            }
            QProcess _7za_proc;
            _7za_proc.start(_7zaExePath,
                            {QStringLiteral("x"), QDir::toNativeSeparators(zipPath),
                             QStringLiteral("-o") + QDir::toNativeSeparators(extractDir),
                             QStringLiteral("-y")});
            if (!_7za_proc.waitForFinished() || _7za_proc.exitCode() != 0) {
                throw std::runtime_error("Extraction failed");
            }
        }
    }
#else
    proc.start(QStringLiteral("unzip"),
               {QStringLiteral("-o"), zipPath, QStringLiteral("-d"), extractDir});
    if (!proc.waitForFinished() || proc.exitCode() != 0) {
        throw std::runtime_error("Extraction failed");
    }
#endif
}

// Real FCC amateur extract populates far more than this; catches schema-only or aborted imports.
static constexpr qint64 kFccAmatMinRowCount = 5000;

bool fccAmatDatabaseIsValid(const QString &dbPath) {
    if (QFileInfo(dbPath).size() <= 0) {
        return false;
    }

    const QString probeConn =
        QStringLiteral("was_fcc_probe_") + QString::number(QDateTime::currentMSecsSinceEpoch());
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), probeConn);
    db.setDatabaseName(dbPath);
    if (!db.open()) {
        QSqlDatabase::removeDatabase(probeConn);
        return false;
    }

    QSqlQuery q(db);

    if (!q.exec(QStringLiteral("PRAGMA quick_check"))) {
        db.close();
        QSqlDatabase::removeDatabase(probeConn);
        return false;
    }
    if (!q.next()) {
        db.close();
        QSqlDatabase::removeDatabase(probeConn);
        return false;
    }
    const QString quickCheck = q.value(0).toString().trimmed();
    if (quickCheck.compare(QStringLiteral("ok"), Qt::CaseInsensitive) != 0) {
        db.close();
        QSqlDatabase::removeDatabase(probeConn);
        return false;
    }

    if (!q.exec(QStringLiteral("SELECT 1 FROM fcc_amat LIMIT 1"))) {
        db.close();
        QSqlDatabase::removeDatabase(probeConn);
        return false;
    }

    if (!q.exec(QStringLiteral("PRAGMA table_info(fcc_amat)"))) {
        db.close();
        QSqlDatabase::removeDatabase(probeConn);
        return false;
    }
    bool hasCallsign = false;
    bool hasState = false;
    while (q.next()) {
        const QString col = q.value(1).toString();
        if (col.compare(QStringLiteral("callsign"), Qt::CaseInsensitive) == 0) {
            hasCallsign = true;
        }
        if (col.compare(QStringLiteral("state"), Qt::CaseInsensitive) == 0) {
            hasState = true;
        }
    }
    if (!hasCallsign || !hasState) {
        db.close();
        QSqlDatabase::removeDatabase(probeConn);
        return false;
    }

    if (!q.exec(QStringLiteral("SELECT COUNT(*) FROM fcc_amat"))) {
        db.close();
        QSqlDatabase::removeDatabase(probeConn);
        return false;
    }
    if (!q.next()) {
        db.close();
        QSqlDatabase::removeDatabase(probeConn);
        return false;
    }
    const qint64 rowCount = q.value(0).toLongLong();
    if (rowCount < kFccAmatMinRowCount) {
        db.close();
        QSqlDatabase::removeDatabase(probeConn);
        return false;
    }

    db.close();
    QSqlDatabase::removeDatabase(probeConn);
    return true;
}

} // namespace

auto WASPlugin::updateFccDatabase() -> bool {
    closeDatabase();
    m_lastError.clear();

    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    QString dbPath = dataDir + QStringLiteral("/fcc_amat.sqlite");

    if (QFile::exists(dbPath)) {
        if (fccAmatDatabaseIsValid(dbPath)) {
            return true;
        }
        m_lastError = QStringLiteral("Invalid FCC cache database, rebuilding");
        if (!QFile::remove(dbPath)) {
            m_lastError = QStringLiteral("Cannot remove invalid FCC database: ") + dbPath;
            return false;
        }
    }

    QString zipPath = QDir::tempPath() + QStringLiteral("/l_amat_was.zip");
    QString extractDir = QDir::tempPath() + QStringLiteral("/fcc_extract_was_") +
                         QString::number(QDateTime::currentMSecsSinceEpoch());

    QNetworkAccessManager manager;
    {
        QUrl url(QStringLiteral("https://data.fcc.gov/download/pub/uls/complete/l_amat.zip"));
        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("GLogKit-WAS-Plugin/1.0"));
        QNetworkReply *reply = manager.get(req);
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        if (reply->error() != QNetworkReply::NoError) {
            m_lastError = QStringLiteral("Download failed: ") + reply->errorString();
            reply->deleteLater();
            return false;
        }
        QFile zf(zipPath);
        if (!zf.open(QIODevice::WriteOnly)) {
            m_lastError = QStringLiteral("Cannot write zip file");
            reply->deleteLater();
            return false;
        }
        zf.write(reply->readAll());
        zf.close();
        reply->deleteLater();
    }

    try {
        extractZipWas(zipPath, extractDir, manager);
    } catch (const std::exception &e) {
        m_lastError = QString::fromLatin1(e.what());
        QDir(extractDir).removeRecursively();
        return false;
    } catch (...) {
        m_lastError = QStringLiteral("Extract failed");
        QDir(extractDir).removeRecursively();
        return false;
    }

    QString enPath = extractDir + QStringLiteral("/EN.dat");
    QString hdPath = extractDir + QStringLiteral("/HD.dat");
    if (!QFile::exists(enPath) || !QFile::exists(hdPath)) {
        m_lastError = QStringLiteral("EN.dat or HD.dat missing after extract");
        QDir(extractDir).removeRecursively();
        return false;
    }

    QString m_connName = connName + QStringLiteral("_build");
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connName);
        db.setDatabaseName(dbPath);
        if (!db.open()) {
            m_lastError = QStringLiteral("Cannot create database: ") + db.lastError().text();
            QSqlDatabase::removeDatabase(m_connName);
            QDir(extractDir).removeRecursively();
            return false;
        }

        QSqlQuery query(db);
        query.exec(QStringLiteral("PRAGMA synchronous = OFF;"));
        query.exec(QStringLiteral("PRAGMA journal_mode = MEMORY;"));
        query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS fcc_amat ("
                                  "callsign TEXT PRIMARY KEY, "
                                  "state TEXT NOT NULL)"));

        QHash<long long, QString> usiToState;
        QFile enFile(enPath);
        if (!enFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_lastError = QStringLiteral("Cannot open EN.dat");
            db.close();
            QSqlDatabase::removeDatabase(m_connName);
            QDir(extractDir).removeRecursively();
            return false;
        }

        QTextStream enStream(&enFile);
        while (!enStream.atEnd()) {
            QString line = enStream.readLine();
            if (!line.startsWith(QStringLiteral("EN|"))) {
                continue;
            }

            QStringList fields = line.split('|');
            if (fields.size() < 18) {
                continue;
            }

            long long usi = fields[1].toLongLong();
            if (usi <= 0) {
                continue;
            }

            QString stateCandidate = fields[17].trimmed();
            if (stateCandidate.length() == 2 && stateCandidate == stateCandidate.toUpper()) {
                usiToState.insert(usi, stateCandidate);
            } else {
                for (int i : {12, 16, 17, 18, 19, 20}) {
                    if (i < fields.size()) {
                        QString s = fields[i].trimmed();
                        if (s.length() == 2 && s == s.toUpper()) {
                            usiToState.insert(usi, s);
                            break;
                        }
                    }
                }
            }
        }
        enFile.close();

        QFile hdFile(hdPath);
        if (!hdFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_lastError = QStringLiteral("Cannot open HD.dat");
            db.close();
            QSqlDatabase::removeDatabase(m_connName);
            QDir(extractDir).removeRecursively();
            return false;
        }

        QTextStream hdStream(&hdFile);

        db.transaction();
        query.prepare(
            QStringLiteral("INSERT OR REPLACE INTO fcc_amat (callsign, state) VALUES (?, ?)"));

        int hdCount = 0;
        while (!hdStream.atEnd()) {
            QString line = hdStream.readLine();
            if (line.isEmpty()) {
                continue;
            }

            QStringList fields = line.split('|');
            if (fields.size() < 6) {
                continue;
            }

            bool ok = false;
            long long usi = fields[1].toLongLong(&ok);
            if (!ok || !usiToState.contains(usi)) {
                continue;
            }

            QString callsign = fields[4].trimmed().toUpper();
            QString status = fields[5].trimmed();
            if (callsign.isEmpty() || status != QStringLiteral("A")) {
                continue;
            }

            query.addBindValue(callsign);
            query.addBindValue(usiToState[usi]);
            if (!query.exec()) {
                qWarning() << "Failed to insert" << callsign << query.lastError().text();
            }

            ++hdCount;
            if (hdCount % 10000 == 0) {
                db.commit();
                db.transaction();
            }
        }
        db.commit();
        hdFile.close();

        query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_fcc_call ON fcc_amat(callsign)"));
        db.close();
    }
    QSqlDatabase::removeDatabase(m_connName);
    QDir(extractDir).removeRecursively();
    return true;
}

auto WASPlugin::openDatabase() -> bool {
    if (QSqlDatabase::contains(connName)) {
        QSqlDatabase db = QSqlDatabase::database(connName);
        if (db.isOpen()) {
            m_query = QSqlQuery(db);
            m_query.prepare("SELECT state FROM fcc_amat WHERE callsign = ?");
            return true;
        }
        QSqlDatabase::removeDatabase(connName);
    }

    QString dbPath =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/fcc_amat.sqlite";
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
    db.setDatabaseName(dbPath);
    if (!db.open()) {
        m_lastError = "Failed to open database: " + db.lastError().text();
        return false;
    }

    m_query = QSqlQuery(db);
    m_query.prepare("SELECT state FROM fcc_amat WHERE callsign = ?");
    return true;
}

void WASPlugin::closeDatabase() {
    if (QSqlDatabase::contains(connName)) {
        m_query.clear();
        m_query = QSqlQuery();

        QSqlDatabase db = QSqlDatabase::database(connName);
        if (db.isOpen()) {
            db.close();
        }
        QSqlDatabase::removeDatabase(connName);
    }
}

auto WASPlugin::queryStateByCallsign(const QString &callsign) -> QString {
    QString *cached = m_cache.object(callsign);
    if (cached != nullptr) {
        return *cached;
    }

    m_query.addBindValue(callsign.toUpper());
    if (m_query.exec() && m_query.next()) {
        QString state = m_query.value(0).toString().trimmed().toUpper();
        m_cache.insert(callsign, new QString(state), 1);
        return state;
    }

    m_cache.insert(callsign, new QString(""), 1);
    return {};
}

auto WASPlugin::parseCallsignFromADIF(const QString &adif) -> QString {
    QRegularExpression re(R"(<CALL(?::\d+)?>([^<]+))", QRegularExpression::CaseInsensitiveOption);
    auto match = re.match(adif);
    if (match.hasMatch()) {
        return match.captured(1).trimmed().toUpper();
    }
    return {};
}

auto WASPlugin::parseStateFromADIF(const QString &adif) -> QString {
    QRegularExpression re(R"(<STATE(?::\d+)?>([^<]+))", QRegularExpression::CaseInsensitiveOption);
    auto match = re.match(adif);
    if (match.hasMatch()) {
        QString state = match.captured(1).trimmed().toUpper();
        if (isValidUSState(state)) {
            return state;
        }
    }
    return {};
}

auto WASPlugin::isValidUSState(const QString &state) const -> bool {
    return US_STATES.contains(state);
}

extern "C" {

auto pluginName() -> const char * { return instance.pluginName(); }

auto install() -> bool { return instance.install(); }

auto uninstall() -> bool { return instance.uninstall(); }

auto beforeEvaluate() -> bool { return instance.beforeEvaluate(); }

auto afterEvaluate() -> bool { return instance.afterEvaluate(); }

auto evaluate(const IGRecord *QSO, IGRecordGetValueByField callback) -> bool {
    return instance.evaluate(QSO, callback);
}

void getResult(char *result_buf, uint64_t *result_len, uint64_t max_result_len) {
    instance.getResult(result_buf, result_len, max_result_len);
}

void getLastError(char *result_buf, uint64_t *result_len, uint64_t max_result_len) {
    instance.getLastError(result_buf, result_len, max_result_len);
}
}