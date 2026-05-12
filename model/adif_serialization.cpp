#include "adif_serialization.h"

#include <QString>
#include <QTextStream>
#include <sstream>

namespace AdifSerialization {

void toCsv(QTextStream &stream, const std::vector<GRecord> &p_records,
           const std::vector<std::string> &p_rheaders) {
    auto iter1 = p_rheaders.begin();
    while (true) {
        stream << QString::fromStdString(*iter1);
        ++iter1;
        if (iter1 == p_rheaders.end()) {
            break;
        }
        stream << ',';
    }
    stream << "\n";

    auto iter2 = p_records.begin();
    while (true) {
        const auto &record = *iter2;
        for (auto h = p_rheaders.begin(); h != p_rheaders.end(); ++h) {
            if (h != p_rheaders.begin()) {
                stream << ',';
            }
            auto iter3 = record.find(*h);
            if (iter3 == record.end() || !iter3->second) {
                continue;
            }
            stream << QString::fromUtf8(iter3->second->asView());
        }
        ++iter2;
        if (iter2 == p_records.end()) {
            break;
        }
        stream << "\n";
    }
}

void toAdif(QTextStream &stream, const std::vector<GRecord> &p_records) {
    for (const auto &record : p_records) {
        std::ostringstream oss;
        oss << record;
        stream << QString::fromStdString(oss.str()) << "\n";
    }
}

} // namespace AdifSerialization
