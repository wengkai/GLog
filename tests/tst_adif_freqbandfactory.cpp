#include <QtTest>

#include <memory>
#include <string>

#include "AdifFreqBandFactory.h"

class TestAdifFreqBandFactory : public QObject {
    Q_OBJECT

  private slots:
    void test_check_invalid_and_safeStod_overflow();
    void test_create_switchPending_and_band_pending_swap();
    void test_AdifFreq_take_rawEmpty_export_switchPending_compat_and_finalFail();
    void test_AdifFreq_take_peerExpired();
    void test_AdifFreq_take_invalid_input_sets_pending();
    void test_AdifBand_take_empty_invalid_pending_and_peerExpired();
};

void TestAdifFreqBandFactory::test_check_invalid_and_safeStod_overflow() {
    QVERIFY(AdifFreqBandFactory::check("3.6", "80M"));

    // Invalid number format.
    QVERIFY(!AdifFreqBandFactory::check("12x", "80M"));

    // Invalid band.
    QVERIFY(!AdifFreqBandFactory::check("3.6", "NO_SUCH_BAND"));

    // safeStod overflow: valid digits, but from_chars to double fails/range errors.
    std::string hugeDigits(420, '9'); // digits-only, passes AdifNumber::check
    QVERIFY(!AdifFreqBandFactory::check(hugeDigits, "80M"));
}

void TestAdifFreqBandFactory::test_create_switchPending_and_band_pending_swap() {
    // Out of band: "10.0" is NOT within 20M range [14.0..14.35]
    auto [freqPtr, bandPtr] = AdifFreqBandFactory::createFreqBandPair("10.0", "20M");
    QVERIFY(freqPtr);
    QVERIFY(bandPtr);

    // createFreqBandPair should call bandPtr->switchPending().
    QVERIFY(bandPtr->get().empty());
    QCOMPARE(QString::fromStdString(bandPtr->getPendingValue()), QStringLiteral("20M"));
}

void TestAdifFreqBandFactory::
    test_AdifFreq_take_rawEmpty_export_switchPending_compat_and_finalFail() {
    // Create a state where bandPeer->m_rawValue is empty (switchPending already happened).
    auto [freqPtr, bandPtr] = AdifFreqBandFactory::createFreqBandPair("10.0", "20M");
    QVERIFY(bandPtr->get().empty());

    // raw empty export: bandPeer->m_rawValue.empty()
    const auto res1 = freqPtr->take(std::string("3.6")); // 80M
    QVERIFY(res1.flag);
    QCOMPARE(QString::fromStdString(freqPtr->get()), QStringLiteral("3.6"));
    QCOMPARE(QString::fromStdString(bandPtr->get()), QStringLiteral("80M"));
    QCOMPARE(QString::fromStdString(bandPtr->getPendingValue()),
             QStringLiteral("20M")); // pending is unchanged in this branch

    // compatCurrent: in bandPeer->m_rawValue
    const auto res2 = freqPtr->take(std::string("3.7")); // still 80M
    QVERIFY(res2.flag);
    QCOMPARE(QString::fromStdString(freqPtr->get()), QStringLiteral("3.7"));
    QCOMPARE(QString::fromStdString(bandPtr->get()), QStringLiteral("80M"));
    QCOMPARE(QString::fromStdString(bandPtr->getPendingValue()), QStringLiteral("20M"));

    // compatPending: in bandPeer->m_pendingValue (20M), but NOT in m_rawValue (80M)
    const auto res3 = freqPtr->take(std::string("14.2")); // 20M
    QVERIFY(res3.flag);
    QCOMPARE(QString::fromStdString(freqPtr->get()), QStringLiteral("14.2"));
    QCOMPARE(QString::fromStdString(bandPtr->get()), QStringLiteral("20M"));
    QVERIFY(bandPtr->getPendingValue().empty());

    // final-fail: not compatible with m_rawValue (20M) nor with m_pendingValue (empty)
    const std::string prevRaw = freqPtr->get();
    const auto res4 = freqPtr->take(std::string("7.1")); // 40M
    QVERIFY(!res4.flag);
    QVERIFY(!res4.failed_store.has_value());
    QCOMPARE(freqPtr->get(), prevRaw);
}

void TestAdifFreqBandFactory::test_AdifFreq_take_peerExpired() {
    auto [freqPtr, bandPtr] = AdifFreqBandFactory::createFreqBandPair("3.6", "80M");
    QVERIFY(freqPtr);
    QVERIFY(bandPtr);

    QVERIFY(!bandPtr->get().empty());
    bandPtr.reset(); // expire weak_ptr inside AdifFreq

    const auto res = freqPtr->take(std::string("3.7"));
    QVERIFY(res.flag);
    QVERIFY(!res.failed_store.has_value());
    QCOMPARE(QString::fromStdString(freqPtr->get()), QStringLiteral("3.7"));
    QVERIFY(freqPtr->getPendingValue().empty());
}

void TestAdifFreqBandFactory::test_AdifFreq_take_invalid_input_sets_pending() {
    auto [freqPtr, bandPtr] = AdifFreqBandFactory::createFreqBandPair("3.6", "80M");
    (void)bandPtr;

    const std::string prevRaw = freqPtr->get();

    // Invalid input (AdifNumber::check fails): should set pending + return false.
    const auto res1 = freqPtr->take(std::string("12x"));
    QVERIFY(!res1.flag);
    QVERIFY(!res1.failed_store.has_value());
    QCOMPARE(QString::fromStdString(freqPtr->get()), QString::fromStdString(prevRaw));
    QCOMPARE(QString::fromStdString(freqPtr->getPendingValue()), QStringLiteral("12x"));

    // parseFrequency fail path (safeStod overflow): should set pending + return false.
    std::string hugeDigits(420, '9');
    const auto res2 = freqPtr->take(std::move(hugeDigits));
    QVERIFY(!res2.flag);
    QVERIFY(!res2.failed_store.has_value());
    QCOMPARE(QString::fromStdString(freqPtr->get()), QString::fromStdString(prevRaw));
    QVERIFY(!freqPtr->getPendingValue().empty());
}

void TestAdifFreqBandFactory::test_AdifBand_take_empty_invalid_pending_and_peerExpired() {
    // newValue.empty(): clear raw + pending.
    {
        auto [freqPtr, bandPtr] = AdifFreqBandFactory::createFreqBandPair("3.6", "80M");
        (void)freqPtr;
        const auto res = bandPtr->take(std::string{});
        QVERIFY(res.flag);
        QVERIFY(bandPtr->get().empty());
        QVERIFY(bandPtr->getPendingValue().empty());
    }

    // Invalid band pending: isValidBand(normValue) fails => m_pendingValue set + return false.
    {
        auto [freqPtr, bandPtr] = AdifFreqBandFactory::createFreqBandPair("3.6", "80M");
        (void)freqPtr;
        const std::string prevRaw = bandPtr->get();

        const auto resBad = bandPtr->take(std::string("no_such_band"));
        QVERIFY(!resBad.flag);
        QCOMPARE(QString::fromStdString(bandPtr->get()), QString::fromStdString(prevRaw));
        QCOMPARE(QString::fromStdString(bandPtr->getPendingValue()),
                 QStringLiteral("NO_SUCH_BAND"));
    }

    // freqPeer expired: should set pending + return false.
    {
        auto [freqPtr, bandPtr] = AdifFreqBandFactory::createFreqBandPair("3.6", "80M");
        QVERIFY(!bandPtr->get().empty());
        freqPtr.reset();

        const auto res = bandPtr->take(std::string("80M"));
        QVERIFY(!res.flag);
        QCOMPARE(QString::fromStdString(bandPtr->get()),
                 QStringLiteral("80M")); // raw stays unchanged
        QCOMPARE(QString::fromStdString(bandPtr->getPendingValue()), QStringLiteral("80M"));
    }

    // compat raw success (freqPeer->m_rawValue in target band)
    {
        auto [freqPtr, bandPtr] = AdifFreqBandFactory::createFreqBandPair("3.6", "80M");
        QVERIFY(freqPtr);
        const auto res = bandPtr->take(std::string("80M"));
        QVERIFY(res.flag);
        QVERIFY(!res.failed_store.has_value());
        QCOMPARE(QString::fromStdString(bandPtr->get()), QStringLiteral("80M"));
        QVERIFY(bandPtr->getPendingValue().empty());
    }
}

QTEST_MAIN(TestAdifFreqBandFactory)
#include "tst_adif_freqbandfactory.moc"
