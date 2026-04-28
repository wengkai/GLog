#include "was_plugin.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QTextStream>
#include "was_plugin.h"

WASPlugin instance;

// 美国50个州的缩写列表
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

const char *WASPlugin::pluginName() const noexcept {
    // 返回静态字符串，不需要 deleteString 释放
    return "WAS Plugin (FCC Database)";
}

bool WASPlugin::install() noexcept {
    try {
        return updateFccDatabase();
    } catch (...) {
        m_lastError = "Unknown exception during install";
        return false;
    }
}

bool WASPlugin::uninstall() noexcept {
    try {
        closeDatabase();
        // 删除数据库文件
        QString dbPath =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/fcc_amat.sqlite";
        QFile::remove(dbPath);
        return true;
    } catch (...) {
        m_lastError = "Unknown exception during uninstall";
        return false;
    }
}

bool WASPlugin::beforeEvaluate() noexcept {
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

bool WASPlugin::afterEvaluate() noexcept {
    try {
        // 生成结果并缓存
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

bool WASPlugin::evaluate(const IGRecord *QSO, IGRecordGetValueByField callback) noexcept {
    try {
        if (!QSO || !callback) {
            return false;
        }
        auto getValueByField = [=](const char *field, uint64_t field_len, char *result_buf,
                                   uint64_t *result_len, uint64_t max_result_len) {
            return callback(QSO, field, field_len, result_buf, result_len, max_result_len);
        };

        // 1. 获取呼号字段
        char callBuf[64] = {0};
        uint64_t callLen = 0;
        auto res = getValueByField("call", 4, callBuf, &callLen, sizeof(callBuf) - 1);
        if (res != IGRecord::Result::NoError || callLen == 0) {
            return false; // 无呼号，跳过
        }
        callBuf[callLen] = '\0';
        QString callsign = QString::fromUtf8(callBuf).trimmed().toUpper();

        // 2. 先尝试从记录中直接获取州字段
        char stateBuf[8] = {0};
        uint64_t stateLen = 0;
        QString state;
        res = getValueByField("state", 5, stateBuf, &stateLen, sizeof(stateBuf) - 1);
        if (res == IGRecord::Result::NoError && stateLen > 0) {
            stateBuf[stateLen] = '\0';
            state = QString::fromUtf8(stateBuf).trimmed().toUpper();
        }

        // 3. 若没有州字段，查询数据库
        if (state.isEmpty() && !callsign.isEmpty()) {
            state = queryStateByCallsign(callsign);
        }

        // 4. 验证并记录
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
            // 无结果：长度为0，缓冲区（若有效）置为空字符串
            if (result_len) {
                *result_len = 0;
            }
            if (result_buf && max_result_len > 0) {
                result_buf[0] = '\0';
            }
            return;
        }

        // 将结果转换为 UTF-8 字节序列
        QByteArray utf8 = m_cachedResult.toUtf8();
        uint64_t actual_len = static_cast<uint64_t>(utf8.size());

        // 输出实际长度（不含结尾空字符），即使缓冲区不足也返回完整长度
        if (result_len) {
            *result_len = actual_len;
        }

        // 向缓冲区复制数据，留一个字节给结尾空字符
        if (result_buf && max_result_len > 0) {
            uint64_t copy_len = std::min(actual_len, max_result_len - 1);
            std::memcpy(result_buf, utf8.constData(), copy_len);
            result_buf[copy_len] = '\0';
        }
    } catch (...) {
        // 发生异常时同样保证输出状态一致
        if (result_len) {
            *result_len = 0;
        }
        if (result_buf && max_result_len > 0) {
            result_buf[0] = '\0';
        }
    }
}

void WASPlugin::getLastError(char *result_buf, uint64_t *result_len,
                             uint64_t max_result_len) const noexcept {
    try {
        uint64_t full_len = 0; // 完整错误信息的 UTF-8 字节数（不含 '\0'）
        if (!m_lastError.isEmpty()) {
            QByteArray utf8 = m_lastError.toUtf8();
            full_len = static_cast<uint64_t>(utf8.size());
        }

        // 无论 result_buf 是否为空，只要 result_len 有效，都必须设置长度
        if (result_len) {
            *result_len = full_len;
        }

        // 如果 result_buf 为空，跳过写入操作
        if (!result_buf || max_result_len == 0) {
            return;
        }

        // 写入缓冲区，确保不越界且字符串以 '\0' 结尾
        if (full_len == 0) {
            result_buf[0] = '\0';
        } else {
            QByteArray utf8 = m_lastError.toUtf8();
            uint64_t copy_len = full_len;
            if (copy_len >= max_result_len) {
                copy_len = max_result_len - 1; // 留出 '\0' 空间
            }
            std::memcpy(result_buf, utf8.constData(), copy_len);
            result_buf[copy_len] = '\0';
        }
    } catch (...) {
        // 异常情况：安全地设置 result_len，并尽可能使缓冲区有效
        if (result_len) {
            *result_len = 0;
        }
        if (result_buf && max_result_len > 0) {
            result_buf[0] = '\0';
        }
    }
}

bool WASPlugin::updateFccDatabase() {
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    QString dbPath = dataDir + "/fcc_amat.sqlite";

    // 如果数据库已存在，可选择跳过（也可增加版本检查）
    if (QFile::exists(dbPath)) {
        return true;
    }

    // 1. 定位 EN.dat 和 HD.dat 文件
    QString appDir = QCoreApplication::applicationDirPath();
    QString enPath = appDir + "/EN.dat";
    QString hdPath = appDir + "/HD.dat";

    // 若默认路径不存在，尝试在数据目录查找
    if (!QFile::exists(enPath)) {
        enPath = QStandardPaths::locate(QStandardPaths::AppDataLocation, "EN.dat");
    }
    if (!QFile::exists(hdPath)) {
        hdPath = QStandardPaths::locate(QStandardPaths::AppDataLocation, "HD.dat");
    }

    QFile enFile(enPath);
    QFile hdFile(hdPath);
    if (!enFile.exists() || !hdFile.exists()) {
        m_lastError = "FCC data files (EN.dat, HD.dat) not found. "
                      "Please place them in the application directory.";
        return false;
    }

    // 2. 创建数据库连接（使用临时连接名）
    QString m_connName = connName + "_build";
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connName);
        db.setDatabaseName(dbPath);
        if (!db.open()) {
            m_lastError = "Cannot create database: " + db.lastError().text();
            return false;
        }

        // 性能优化：关闭同步，使用内存日志
        QSqlQuery query(db);
        query.exec("PRAGMA synchronous = OFF;");
        query.exec("PRAGMA journal_mode = MEMORY;");
        query.exec("CREATE TABLE IF NOT EXISTS fcc_amat ("
                   "callsign TEXT PRIMARY KEY, "
                   "state TEXT NOT NULL)");

        // 3. 第一遍：解析 EN.dat，建立 USI → State 映射
        QHash<long long, QString> usiToState;
        if (!enFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_lastError = "Cannot open EN.dat";
            return false;
        }

        int enCount = 0;
        QTextStream enStream(&enFile);
        // enStream.setCodec("UTF-8");
        while (!enStream.atEnd()) {
            QString line = enStream.readLine();
            if (!line.startsWith("EN|")) {
                continue;
            }

            QStringList fields = line.split('|');
            if (fields.size() < 18) {
                continue;
            }

            long long usi = fields[1].toLongLong();
            if (usi <= 0) continue;

            // 字段17通常是州缩写
            QString stateCandidate = fields[17].trimmed();
            if (stateCandidate.length() == 2 && stateCandidate == stateCandidate.toUpper()) {
                usiToState.insert(usi, stateCandidate);
                ++enCount;
            } else {
                // 若17不是州，则向后搜索包含两字母大写的字段
                for (int i : {12, 16, 18, 19, 20}) {
                    if (i < fields.size()) {
                        QString s = fields[i].trimmed();
                        if (s.length() == 2 && s == s.toUpper()) {
                            usiToState.insert(usi, s);
                            ++enCount;
                            break;
                        }
                    }
                }
            }
        }
        enFile.close();

        // 4. 第二遍：解析 HD.dat，根据 USI 和状态 'A' 插入呼号
        if (!hdFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_lastError = "Cannot open HD.dat";
            return false;
        }

        QTextStream hdStream(&hdFile);
        // hdStream.setCodec("UTF-8");

        db.transaction();
        query.prepare("INSERT OR REPLACE INTO fcc_amat (callsign, state) VALUES (?, ?)");

        int hdCount = 0;
        while (!hdStream.atEnd()) {
            QString line = hdStream.readLine();
            if (line.isEmpty()) continue;

            QStringList fields = line.split('|');
            if (fields.size() < 6) continue; // 确保索引 5 (status) 安全

            bool ok;
            long long usi = fields[1].toLongLong(&ok);
            if (!ok || !usiToState.contains(usi)) continue;

            QString callsign = fields[4].trimmed().toUpper();
            QString status = fields[5].trimmed();
            // 仅处理活动执照 (status == "A")
            if (callsign.isEmpty() || status != "A") continue;

            query.addBindValue(callsign);
            query.addBindValue(usiToState[usi]);
            if (!query.exec()) {
                qWarning() << "Failed to insert" << callsign << query.lastError().text();
            }

            ++hdCount;
            // 每 10000 条提交一次事务
            if (hdCount % 10000 == 0) {
                db.commit();
                db.transaction();
            }
        }
        db.commit();
        hdFile.close();

        // 5. 创建索引加速查询
        query.exec("CREATE INDEX IF NOT EXISTS idx_fcc_call ON fcc_amat(callsign)");
        db.close();
    }
    QSqlDatabase::removeDatabase(m_connName);
    return true;
}

bool WASPlugin::openDatabase() {
    if (QSqlDatabase::contains(connName)) {
        QSqlDatabase db = QSqlDatabase::database(connName);
        if (db.isOpen()) {
            m_query = QSqlQuery(db);
            // ✅ 确保预处理语句在连接打开后立即准备
            m_query.prepare("SELECT state FROM fcc_amat WHERE callsign = ?");
            return true;
        } else {
            // 连接存在但未打开，移除后重新创建
            QSqlDatabase::removeDatabase(connName);
        }
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
        // ✅ 显式清除查询对象，释放数据库资源
        m_query.clear();
        m_query = QSqlQuery(); // 赋空查询

        QSqlDatabase db = QSqlDatabase::database(connName);
        if (db.isOpen()) {
            db.close();
        }
        QSqlDatabase::removeDatabase(connName);
    }
}

QString WASPlugin::queryStateByCallsign(const QString &callsign) {
    // ✅ 先检查缓存
    QString *cached = m_cache.object(callsign);
    if (cached) {
        return *cached;
    }

    // ✅ 直接绑定并执行，不再依赖 isActive()
    m_query.addBindValue(callsign.toUpper());
    if (m_query.exec() && m_query.next()) {
        QString state = m_query.value(0).toString().trimmed().toUpper();
        m_cache.insert(callsign, new QString(state), 1);
        return state;
    }

    // 未找到时也缓存空字符串，避免重复查询
    m_cache.insert(callsign, new QString(""), 1);
    return QString();
}

QString WASPlugin::parseCallsignFromADIF(const QString &adif) {
    // ✅ 支持 <CALL:len> 和 <CALL> 两种形式
    QRegularExpression re(R"(<CALL(?::\d+)?>([^<]+))", QRegularExpression::CaseInsensitiveOption);
    auto match = re.match(adif);
    if (match.hasMatch()) {
        return match.captured(1).trimmed().toUpper();
    }
    return QString();
}

QString WASPlugin::parseStateFromADIF(const QString &adif) {
    QRegularExpression re(R"(<STATE(?::\d+)?>([^<]+))", QRegularExpression::CaseInsensitiveOption);
    auto match = re.match(adif);
    if (match.hasMatch()) {
        QString state = match.captured(1).trimmed().toUpper();
        if (isValidUSState(state)) {
            return state;
        }
    }
    return QString();
}

bool WASPlugin::isValidUSState(const QString &state) const { return US_STATES.contains(state); }

extern "C" {

const char *pluginName() { return instance.pluginName(); }

bool install() { return instance.install(); }

bool uninstall() { return instance.uninstall(); }

bool beforeEvaluate() { return instance.beforeEvaluate(); }

bool afterEvaluate() { return instance.afterEvaluate(); }

bool evaluate(const IGRecord *QSO, IGRecordGetValueByField callback) {
    return instance.evaluate(QSO, callback);
}

void getResult(char *result_buf, uint64_t *result_len, uint64_t max_result_len) {
    instance.getResult(result_buf, result_len, max_result_len);
}

void getLastError(char *result_buf, uint64_t *result_len, uint64_t max_result_len) {
    instance.getLastError(result_buf, result_len, max_result_len);
}
}