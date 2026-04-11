#include "ctydb.h"
#include <QFile>
#include <QUrl>
#include <algorithm>
#include <exception>

CtyDB::CtyDB(QObject *parent) : QObject(parent) {}

std::pair<bool, QString /*error msg*/> CtyDB::loadLocalDB(const QString &filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {false, tr("Can not open file.")};
    }
    auto ret = loadDB(file, QUrl::fromLocalFile(filename).toString());
    file.close();
    return ret;
}

std::pair<bool, QString /*error msg*/> CtyDB::loadDB(QIODevice &device, const QString &db_hint) {
    auto cty = QString(device.readAll());
    return loadDBString(cty, db_hint);
}

std::pair<bool, QString /*error msg*/> CtyDB::loadDBString(const QString &cty,
                                                           const QString &db_hint) {
    emit loadDbBegin();
    std::unique_lock<decltype(mutex)> lock(mutex);
    auto blocks = cty.split(";");
    if (blocks.isEmpty()) {
        return {false, tr("Parse failed.")};
    }
    _clear();
    try {
        for (auto &block : blocks) {
            auto s_block = block.split(":");
            if (s_block.size() < 9) {
                continue;
            }
            auto ent = std::make_shared<CtyEntry>();
            ent->vaild = true;
            ent->name = s_block[0].trimmed();
            ent->cq = s_block[1].trimmed().toInt();
            ent->itu = s_block[2].trimmed().toInt();
            ent->continent = s_block[3].trimmed();
            ent->lat = s_block[4].trimmed().toDouble();
            ent->lon =
                -s_block[5].trimmed().toDouble(); // Longitude in degrees, + for West, not East
            ent->utc_offset = s_block[6].trimmed().toDouble();
            ent->p_prefix = s_block[7].trimmed();
            ent->ARRL_sponsored = !ent->p_prefix.startsWith("*");
            auto patterns = s_block[8].trimmed().split(",");
            for (auto &pattern : patterns) {
                auto m_pattern = pattern.trimmed().toUpper().replace("\n", "").replace("\r", "");
                if (m_pattern.isEmpty()) {
                    continue;
                }
                std::size_t prefixEnd = 0;
                auto my_ent = overrideEnt(m_pattern, ent, prefixEnd);
                auto it = location_map.find(
                    {my_ent->lat, my_ent->lon}); // std::map<std::pair<double, double>, int>
                if (it == location_map.end()) {
                    my_ent->location_id = int(v_ents.size());
                    v_ents.push_back(my_ent);
                    location_map[{my_ent->lat, my_ent->lon}] = my_ent->location_id;
                } else {
                    my_ent->location_id = it->second;
                }
                m_pattern = m_pattern.left(prefixEnd);
                // https://www.country-files.com/cty-dat-format/
                // Software is expected to parse the file from top to bottom.  If a duplicate
                // callsign is found, simply ignore it the second time
                if (m_pattern.startsWith('=')) {
                    m_pattern = m_pattern.mid(1);
                    if (!m_exact_table.contains(m_pattern)) {
                        m_exact_table[m_pattern] = my_ent;
                    }
                    continue;
                }
                if (!m_prefix_table.contains(m_pattern)) {
                    m_prefix_table[m_pattern] = my_ent;
                }
            }
        }
    } catch (std::runtime_error &e) {
        return {false, tr("Parse failed.")};
    }
    this->db_hint = db_hint;
    m_ready = true;
    lock.unlock();
    emit dbHintChanged(this->db_hint);
    return {true, tr("Success.")};
}

void CtyDB::_clear() {
    m_prefix_table.clear();
    m_exact_table.clear();
    location_map.clear();
    v_ents.clear();
    m_ready = false;
    db_hint.clear();
}

std::pair<std::shared_ptr<CtyEntry>, QString> CtyDB::lookUpCallSign(const QStringView &call) const {
    if (call.isEmpty()) {
        return {std::make_shared<CtyEntry>(), "###NOT-FOUND"};
    }

    auto iter = m_exact_table.find(call);
    if (iter != m_exact_table.end()) {
        return {*iter, ""};
    }

    for (auto len = call.length() - 1; len >= 1; --len) {
        auto iter = m_prefix_table.find(QStringView(call.left(len)));
        if (iter != m_prefix_table.end()) {
            return {(*iter), ""};
        }
    }

    return {std::make_shared<CtyEntry>(), "###NOT-FOUND"};
}

std::pair<std::shared_ptr<CtyEntry>, QString /*hint*/>
CtyDB::lookUpCallSign(const QString &call) const {
    if (call.isEmpty()) {
        return {std::make_shared<CtyEntry>(), "###NOT-FOUND"};
    }

    auto n_call = normalizeCallSign(call);

    return lookUpCallSign(QStringView(n_call));
}

QString CtyDB::normalizeCallSign(const QString &callsign) { return callsign.toUpper().trimmed(); }

void CtyDB::normalizeCallSign(QString &callsign) { callsign = callsign.trimmed().toUpper(); }

// https://www.country-files.com/cty-dat-format/
// The following special characters can be applied after an alias prefix:
// (#)	Override CQ Zone
// [#]	Override ITU Zone
// <#/#>	Override latitude/longitude
// {aa}	Override Continent
// ~#~	Override local time offset from GMT

std::shared_ptr<CtyEntry> CtyDB::overrideEnt(const QString &m_pattern,
                                             std::shared_ptr<CtyEntry> ent,
                                             std::size_t &prefixEnd) {
    prefixEnd = static_cast<std::size_t>(m_pattern.size());

    if (!m_pattern.contains('(') && !m_pattern.contains('[') && !m_pattern.contains('<') &&
        !m_pattern.contains('{') && !m_pattern.contains('~')) {
        return ent;
    }

    auto my_ent = std::make_shared<CtyEntry>(*ent);

    // CQ Zone: (n)
    if (auto start = m_pattern.indexOf('('); start != -1) {
        auto end = m_pattern.indexOf(')', start);
        if (end == -1) {
            throw std::runtime_error("Invalid pattern: missing )");
        }
        my_ent->cq = m_pattern.mid(start + 1, end - start - 1).toInt();
        prefixEnd = std::min(prefixEnd, std::size_t(start));
    }

    // ITU Zone: [n]
    if (auto start = m_pattern.indexOf('['); start != -1) {
        auto end = m_pattern.indexOf(']', start);
        if (end == -1) {
            throw std::runtime_error("Invalid pattern: missing ]");
        }
        my_ent->itu = m_pattern.mid(start + 1, end - start - 1).toInt();
        prefixEnd = std::min(prefixEnd, std::size_t(start));
    }

    // Lat/Lon: <lat/lon>
    if (auto start = m_pattern.indexOf('<'); start != -1) {
        auto mid = m_pattern.indexOf('/', start);
        auto end = m_pattern.indexOf('>', start);
        if (mid == -1 || end == -1) {
            throw std::runtime_error("Invalid pattern: missing / or >");
        }

        my_ent->lat = m_pattern.mid(start + 1, mid - start - 1).toDouble();
        my_ent->lon = -m_pattern.mid(mid + 1, end - mid - 1).toDouble();
        prefixEnd = std::min(prefixEnd, std::size_t(start));
    }

    // Continent: {aa}
    if (auto start = m_pattern.indexOf('{'); start != -1) {
        auto end = m_pattern.indexOf('}', start);
        if (end == -1) {
            throw std::runtime_error("Invalid pattern: missing }");
        }
        my_ent->continent = m_pattern.mid(start + 1, end - start - 1);
        prefixEnd = std::min(prefixEnd, std::size_t(start));
    }

    // UTC Offset: ~#~
    if (auto start = m_pattern.indexOf('~'); start != -1) {
        auto end = m_pattern.indexOf('~', start + 1);
        if (end == -1) {
            throw std::runtime_error("Invalid pattern: missing second ~");
        }
        my_ent->utc_offset = m_pattern.mid(start + 1, end - start - 1).toDouble();
        prefixEnd = std::min(prefixEnd, std::size_t(start));
    }

    return my_ent;
}

CtyDB *CtyDB::instance() {
    static auto *i = new CtyDB;
    return i;
}
