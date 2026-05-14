#include "award_entity_count_report.h"

#include <QCoreApplication>
#include <QStringList>

#include <map>
#include <memory>
#include <shared_mutex>

#include "LoadedAwardPlugin.h"
#include "ctydb.h"
#include "record.h"

namespace {

QString pluginEvalErrorText(const LoadedAwardPlugin &snap) {
    if (!snap.getLastError) {
        return {};
    }
    uint64_t error_len = 0;
    snap.getLastError(nullptr, &error_len, 0);
    QByteArray buf(static_cast<int>(error_len + 1), '\0');
    snap.getLastError(buf.data(), &error_len, static_cast<uint64_t>(buf.size()));
    return QString::fromUtf8(buf.constData());
}

QString pluginEvalResultText(const LoadedAwardPlugin &snap) {
    uint64_t len = 0;
    snap.getResult(nullptr, &len, 0);
    QByteArray buf(static_cast<int>(len + 1), '\0');
    snap.getResult(buf.data(), &len, static_cast<uint64_t>(buf.size()));
    return QString::fromUtf8(buf.constData());
}

} // namespace

AwardEntityCountReport::AwardEntityCountReport(IAwardPluginsManager *broker) : m_broker(broker) {}

QString AwardEntityCountReport::format(const std::vector<GRecord> &records) const {
    QStringList sections;
    auto *ctydb = CtyDB::instance();
    std::shared_lock<decltype(ctydb->mutex)> lock0(ctydb->mutex);
    std::map<QString, int> counterDXCC;
    std::map<QString, int> counterWAC_ARRL;
    std::map<QString, int> counterWAC_NOTARRL;
    std::map<int, int> counterCQZ;
    QString buf;
    int base_failed_count = 0;
    for (const auto &record : records) {
        auto call = record.at("call")->get();
        buf = QString::fromUtf8(call.data(), qsizetype(call.size()));
        CtyDB::normalizeCallSign(buf);
        auto result = ctydb->lookUpCallSign(QStringView(buf));
        if (!result.first->valid) {
            ++base_failed_count;
            continue;
        }
        if (result.first->ARRL_sponsored) {
            ++counterDXCC[result.first->name];
        }
        if (result.first->ARRL_sponsored) {
            ++counterWAC_ARRL[result.first->continent];
        }
        ++counterWAC_NOTARRL[result.first->continent];
        ++counterCQZ[result.first->cq];
    }
    const QString core = QCoreApplication::translate(
                             "AwardEntityCountReport",
                             "DXCC: %1/100\nWAC (ARRL): %2/6\nWAC (Non-ARRL): %3/6\nCQ: %4/70\n"
                             "%5 Failed to evaluate")
                             .arg(QString::number(counterDXCC.size()))
                             .arg(QString::number(counterWAC_ARRL.size()))
                             .arg(QString::number(counterWAC_NOTARRL.size()))
                             .arg(QString::number(counterCQZ.size()))
                             .arg(base_failed_count);
    sections.append(core);

    auto *broker = m_broker;
    std::unique_ptr<IAwardPluginsProxy> proxy_ptr;
    if (!broker) {
        return sections.join(QLatin1Char('\n'));
    }
    if (proxy_ptr = std::move(broker->getPAwardPlugins()); !proxy_ptr) {
        return sections.join(QLatin1Char('\n'));
    }
    auto &proxy = *proxy_ptr;
    const auto &snapshots = *proxy;
    for (const auto &snap : snapshots) {
        if (!(snap.status->load() == LoadedAwardPlugin::Status::Enabled)) {
            continue;
        }
        int snap_eval_failed = 0;
        const QString pluginTitle =
            snap.pluginName ? QString::fromUtf8(snap.pluginName())
                            : QCoreApplication::translate("AwardEntityCountReport", "Award plugin");
        if (!snap.beforeEvaluate()) {
            sections.append(pluginTitle + QLatin1Char('\n') + pluginEvalErrorText(snap));
            continue;
        }
        for (const auto &record : records) {
            if (!snap.evaluate(&record, &GRecord::getValueByField)) {
                ++snap_eval_failed;
            }
        }
        if (!snap.afterEvaluate()) {
            sections.append(pluginTitle + QLatin1Char('\n') + pluginEvalErrorText(snap));
            continue;
        }
        sections.append(pluginTitle + QLatin1Char('\n') + pluginEvalResultText(snap) +
                        QLatin1Char('\n'));
        sections.append(
            QCoreApplication::translate("AwardEntityCountReport", "%1 Failed to evaluate")
                .arg(snap_eval_failed));
    }

    return sections.join(QStringLiteral("\n\n"));
}
