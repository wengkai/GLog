#include <QtTest>
#include <QCoreApplication>

#include <cstring>
#include <vector>

#include "GLogApplication.h"
#include "LoadedAwardPlugin.h"
#include "award_entity_count_report.h"
#include "ctydb.h"
#include "record.h"

namespace {

static const char kPluginNameA[] = "ReportPluginA";
static const char kPluginNameB[] = "HiddenPlugin";

static const char *stub_plugin_name_a() { return kPluginNameA; }
static const char *stub_plugin_name_b() { return kPluginNameB; }

static bool stub_bool_true() { return true; }

static bool stub_evaluate(const IGRecord *, IGRecordGetValueByField) { return true; }

static void stub_get_empty(char *result_buf, uint64_t *result_len, uint64_t max_result_len) {
    if (result_len) {
        if (result_buf == nullptr) {
            *result_len = 0;
            return;
        }
        *result_len = 0;
    }
    if (result_buf != nullptr && max_result_len > 0) {
        result_buf[0] = '\0';
    }
}

static void stub_get_result_a(char *result_buf, uint64_t *result_len, uint64_t max_result_len) {
    static const char msg[] = "entity_stub_result_a";
    const uint64_t n = static_cast<uint64_t>(strlen(msg));
    if (result_len != nullptr && result_buf == nullptr) {
        *result_len = n;
        return;
    }
    if (result_buf != nullptr && result_len != nullptr && max_result_len > 0) {
        const uint64_t copy = (n < max_result_len) ? n : max_result_len - 1;
        memcpy(result_buf, msg, static_cast<size_t>(copy));
        result_buf[copy] = '\0';
        *result_len = copy;
    }
}

static void stub_get_result_b(char *result_buf, uint64_t *result_len, uint64_t max_result_len) {
    static const char msg[] = "should_not_appear_in_output";
    const uint64_t n = static_cast<uint64_t>(strlen(msg));
    if (result_len != nullptr && result_buf == nullptr) {
        *result_len = n;
        return;
    }
    if (result_buf != nullptr && result_len != nullptr && max_result_len > 0) {
        const uint64_t copy = (n < max_result_len) ? n : max_result_len - 1;
        memcpy(result_buf, msg, static_cast<size_t>(copy));
        result_buf[copy] = '\0';
        *result_len = copy;
    }
}

static LoadedAwardPlugin make_stub_plugin(const char *(*nameFn)(),
                                          void (*getResultFn)(char *, uint64_t *, uint64_t),
                                          LoadedAwardPlugin::Status st) {
    LoadedAwardPlugin p;
    p.filename = QStringLiteral("/tmp/fake_award_plugin_for_report_test");
    p.pluginName = nameFn;
    p.install = stub_bool_true;
    p.uninstall = stub_bool_true;
    p.beforeEvaluate = stub_bool_true;
    p.afterEvaluate = stub_bool_true;
    p.evaluate = stub_evaluate;
    p.getResult = getResultFn;
    p.getLastError = stub_get_empty;
    p.status->store(st);
    return p;
}

class VectorAwardPluginsProxy final : public IAwardPluginsProxy {
    const std::vector<LoadedAwardPlugin> *m_vec;

  public:
    explicit VectorAwardPluginsProxy(const std::vector<LoadedAwardPlugin> *v)
        : IAwardPluginsProxy(), m_vec(v) {}

    const std::vector<LoadedAwardPlugin> *operator->() const override { return m_vec; }
    const std::vector<LoadedAwardPlugin> &operator*() const override { return *m_vec; }
};

struct VectorAwardPluginsBroker final : public IAwardPluginsManager {
    std::vector<LoadedAwardPlugin> m_plugins;

    std::unique_ptr<IAwardPluginsProxy> getPAwardPlugins() override {
        return std::make_unique<VectorAwardPluginsProxy>(&m_plugins);
    }
};

} // namespace

class AwardEntityCountReportTest : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase();
    void test_format_empty();
    void test_format_withQsos();
    void test_format_withEnabledPlugins();

  private:
    bool m_ctyOk = false;
};

void AwardEntityCountReportTest::initTestCase() {
    auto *db = GLogApplication::getCtyDBInstance();
    const auto [ok, err] = db->loadLocalDB();
    m_ctyOk = ok;
    if (!ok) {
        qWarning() << "cty.dat load failed:" << err;
    }
}

void AwardEntityCountReportTest::test_format_empty() {
    if (!m_ctyOk) {
        QSKIP("Built-in cty.dat not available");
    }
    AwardEntityCountReport report(nullptr);
    const QString s = report.format({});
    QVERIFY(s.contains(QLatin1String("DXCC:")));
    QVERIFY(s.contains(QLatin1String("WAC")));
}

void AwardEntityCountReportTest::test_format_withQsos() {
    if (!m_ctyOk) {
        QSKIP("Built-in cty.dat not available");
    }
    std::vector<GRecord> rows;
    GRecord a;
    QVERIFY(a.addOrSetPair("call", "W1AW"));
    rows.push_back(std::move(a));

    AwardEntityCountReport report(nullptr);
    const QString s = report.format(rows);
    QVERIFY(!s.isEmpty());
    QVERIFY(s.contains(QLatin1String("DXCC:")));
}

void AwardEntityCountReportTest::test_format_withEnabledPlugins() {
    if (!m_ctyOk) {
        QSKIP("Built-in cty.dat not available");
    }
    VectorAwardPluginsBroker broker;
    broker.m_plugins.push_back(make_stub_plugin(stub_plugin_name_a, stub_get_result_a,
                                                LoadedAwardPlugin::Status::Enabled));
    broker.m_plugins.push_back(make_stub_plugin(stub_plugin_name_b, stub_get_result_b,
                                                LoadedAwardPlugin::Status::Disabled));

    std::vector<GRecord> rows;
    GRecord a;
    QVERIFY(a.addOrSetPair("call", "W1AW"));
    rows.push_back(std::move(a));

    AwardEntityCountReport report(&broker);
    const QString s = report.format(rows);

    QVERIFY(s.contains(QLatin1String("DXCC:")));
    QVERIFY(s.contains(QLatin1String("\n\n")));
    QVERIFY(s.contains(QLatin1String("ReportPluginA")));
    QVERIFY(s.contains(QLatin1String("entity_stub_result_a")));
    const int idxDxcc = s.indexOf(QLatin1String("DXCC:"));
    const int idxPlugin = s.indexOf(QLatin1String("ReportPluginA"));
    QVERIFY(idxPlugin > idxDxcc);

    QVERIFY(!s.contains(QLatin1String("HiddenPlugin")));
    QVERIFY(!s.contains(QLatin1String("should_not_appear_in_output")));
}

QTEST_MAIN(AwardEntityCountReportTest)
#include "tst_award_entity_count_report.moc"
