#ifndef CTYDB_H
#define CTYDB_H

#include <QList>
#include <QVector>
#include <QString>
#include <QRegularExpression>
#include <QUrl>
#include <QIODevice>
#include <QHash>
#include <variant>
#include <map>
#include <memory>
#include <atomic>
#include <set>
#include <shared_mutex>

struct CtyEntry
{
    int location_id = -1;
    bool vaild = false;
    QString name;                       // Entity Name
    int cq;                             // CQ Zone
    int itu;                            // ITU Zone
    QString continent;                  // Continent
    qreal lat;                          // Latitude (Degrees North)
    qreal lon;                          // Longitude (Degrees East)
    qreal utc_offset;                   // Time Offset from UTC
    QString p_prefix;                   // Primary Prefix
};


class CtyDB : public QObject {

    Q_OBJECT

public:
    static constexpr const char * DEFAULT_ONLINE_DATA = "https://www.country-files.com/bigcty/cty.dat";
    bool loadLocalDB(const QString& filename = QStringLiteral(":/assets/cty.dat"));
    std::pair<std::shared_ptr<CtyEntry>, QString /*hint*/> lookUpCallSign(const QStringView& call) const;
    std::pair<std::shared_ptr<CtyEntry>, QString /*hint*/> lookUpCallSign(const QString& call) const;
    inline std::pair<std::shared_ptr<CtyEntry>, QString /*hint*/> lookUpCallSign(const std::string& call) const {
        return lookUpCallSign(QString::fromUtf8(call.data(), call.size()));
    }
    static QString normalizeCallSign(const QString& call);
    static void normalizeCallSign(QString& call);
    std::shared_ptr<CtyEntry> overrideEnt(const QString& m_pattern, std::shared_ptr<CtyEntry> ent, unsigned int & prefixEnd);
    static CtyDB* instance();
    bool loadDB(QIODevice& device, const QString& db_hint);
    bool ready() const { return m_ready; }
    const std::vector<std::shared_ptr<CtyEntry>> & getVEnts() const { return v_ents; };
    const QString & getDBHint() const { return db_hint; }
    

    std::shared_mutex mutex;

signals:
    void dbHintChanged(QString db_hint);

private:
    void _clear();
    CtyDB(QObject * parent = nullptr);
    QHash<QString, std::shared_ptr<CtyEntry>> m_prefix_table;
    QHash<QString, std::shared_ptr<CtyEntry>> m_exact_table;
    std::map<std::pair<double, double>, int> location_map;
    std::vector<std::shared_ptr<CtyEntry>> v_ents;
    std::atomic<bool> m_ready = false;
    QString db_hint;
    

};

#endif