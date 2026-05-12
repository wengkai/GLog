#include <QtTest>

#include <QCoreApplication>
#include <QSignalSpy>

#include <cstring>

#include "AwardPluginManager.h"
#include "LoadedAwardPlugin.h"

namespace {

static const char kStubPluginName[] = "TestPlugin";

static const char *stub_pluginName() { return kStubPluginName; }

static bool stub_bool_true() { return true; }

static bool stub_bool_false() { return false; }

static bool stub_evaluate(const IGRecord *, IGRecordGetValueByField) { return true; }

static void stub_getResult(char *result_buf, uint64_t *result_len, uint64_t max_result_len) {
    if (result_len) {
        *result_len = 0;
    }
    if (result_buf && max_result_len > 0) {
        result_buf[0] = '\0';
    }
}

static void stub_getLastError_install_fail(char *result_buf, uint64_t *result_len,
                                           uint64_t max_result_len) {
    static const char msg[] = "stub install error";
    const uint64_t n = sizeof(msg) - 1;
    if (result_buf == nullptr && result_len != nullptr) {
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

static LoadedAwardPlugin makeValidFakePlugin() {
    LoadedAwardPlugin p;
    p.filename = QStringLiteral("/tmp/fake_award_plugin.so");
    p.pluginName = stub_pluginName;
    p.install = stub_bool_true;
    p.uninstall = stub_bool_true;
    p.beforeEvaluate = stub_bool_true;
    p.afterEvaluate = stub_bool_true;
    p.evaluate = stub_evaluate;
    p.getResult = stub_getResult;
    p.getLastError = stub_getResult;
    return p;
}

static LoadedAwardPlugin makeFakePluginInstallFails() {
    LoadedAwardPlugin p = makeValidFakePlugin();
    p.install = stub_bool_false;
    p.getLastError = stub_getLastError_install_fail;
    return p;
}

} // namespace

class AwardPluginManagerTest : public QObject {
    Q_OBJECT

  private slots:
    void init() { QCoreApplication::processEvents(); }

    void test_load_after_confirmed_rowCount_index_and_remove();
    void test_remove_invalid_index_noop();
    void test_install_after_confirmed_success_emits_information();
    void test_install_after_confirmed_failure_emits_critical();
    void test_uninstall_after_confirmed_success();
};

void AwardPluginManagerTest::test_load_after_confirmed_rowCount_index_and_remove() {
    AwardPluginManager mgr(nullptr);
    mgr.show();
    QCoreApplication::processEvents();

    QCOMPARE(mgr.rowCount(), size_t(0));

    QSignalSpy infoSpy(&mgr, &AwardPluginManager::userInformation);
    QSignalSpy criticalSpy(&mgr, &AwardPluginManager::userCritical);

    auto plugin = makeValidFakePlugin();
    QVERIFY(plugin.valid());
    mgr.loadAwardPluginAfterConfirmed(std::move(plugin));
    QCoreApplication::processEvents();

    QCOMPARE(criticalSpy.count(), 0);
    QVERIFY(infoSpy.count() >= 1);
    QCOMPARE(mgr.rowCount(), size_t(1));

    const QModelIndex ix = mgr.index(0);
    QVERIFY(ix.isValid());
    QCOMPARE(ix.data(Qt::DisplayRole).toString(), QStringLiteral("TestPlugin"));

    mgr.removeAwardPluginAfterConfirmed(ix);
    QCoreApplication::processEvents();

    QCOMPARE(mgr.rowCount(), size_t(0));
    QVERIFY(infoSpy.count() >= 2);
}

void AwardPluginManagerTest::test_remove_invalid_index_noop() {
    AwardPluginManager mgr(nullptr);
    mgr.show();
    QCoreApplication::processEvents();

    mgr.loadAwardPluginAfterConfirmed(makeValidFakePlugin());
    QCoreApplication::processEvents();
    QCOMPARE(mgr.rowCount(), size_t(1));

    mgr.removeAwardPluginAfterConfirmed(QModelIndex());
    QCoreApplication::processEvents();
    QCOMPARE(mgr.rowCount(), size_t(1));
}

void AwardPluginManagerTest::test_install_after_confirmed_success_emits_information() {
    AwardPluginManager mgr(nullptr);
    mgr.show();
    QCoreApplication::processEvents();

    mgr.loadAwardPluginAfterConfirmed(makeValidFakePlugin());
    QCoreApplication::processEvents();

    QSignalSpy infoSpy(&mgr, &AwardPluginManager::userInformation);
    QSignalSpy criticalSpy(&mgr, &AwardPluginManager::userCritical);

    const QPersistentModelIndex pix(mgr.index(0));
    QVERIFY(pix.isValid());
    mgr.installAwardPluginAfterConfirmed(pix);

    QTRY_VERIFY(infoSpy.count() >= 1);
    QCOMPARE(criticalSpy.count(), 0);
}

void AwardPluginManagerTest::test_install_after_confirmed_failure_emits_critical() {
    AwardPluginManager mgr(nullptr);
    mgr.show();
    QCoreApplication::processEvents();

    mgr.loadAwardPluginAfterConfirmed(makeFakePluginInstallFails());
    QCoreApplication::processEvents();

    QSignalSpy infoSpy(&mgr, &AwardPluginManager::userInformation);
    QSignalSpy criticalSpy(&mgr, &AwardPluginManager::userCritical);

    const QPersistentModelIndex pix(mgr.index(0));
    QVERIFY(pix.isValid());
    mgr.installAwardPluginAfterConfirmed(pix);

    QTRY_VERIFY(criticalSpy.count() >= 1);
    QCOMPARE(infoSpy.count(), 0);
}

void AwardPluginManagerTest::test_uninstall_after_confirmed_success() {
    AwardPluginManager mgr(nullptr);
    mgr.show();
    QCoreApplication::processEvents();

    mgr.loadAwardPluginAfterConfirmed(makeValidFakePlugin());
    QCoreApplication::processEvents();

    const QPersistentModelIndex pix(mgr.index(0));
    mgr.installAwardPluginAfterConfirmed(pix);
    QTRY_VERIFY2(mgr.index(0).data(Qt::DisplayRole).toString().length() > 0, "model settled");

    QSignalSpy infoSpy(&mgr, &AwardPluginManager::userInformation);
    QSignalSpy criticalSpy(&mgr, &AwardPluginManager::userCritical);

    mgr.uninstallAwardPluginAfterConfirmed(pix);
    QTRY_VERIFY(infoSpy.count() >= 1);
    QCOMPARE(criticalSpy.count(), 0);
}

QTEST_MAIN(AwardPluginManagerTest)
#include "tst_awardpluginmanager.moc"
