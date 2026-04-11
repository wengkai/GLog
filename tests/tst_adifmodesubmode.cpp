#include <QtTest>
#include "AdifDataTypes.h"

class TestAdifModeSubMode : public QObject {
    Q_OBJECT

  private slots:

    void createModePair_initialState_data();
    void createModePair_initialState();
    void createModePair_normalization_data();
    void createModePair_normalization();
    void createModePair_invalid_data();
    void createModePair_invalid();

    void submode_set_rejectAndStorePending();
    void submode_set_acceptAndClearPending();
    void submode_set_clearBoth();

    void mode_set_activatesPendingSubmode();
    void mode_set_clearsIncompatibleSubmode();
    void mode_set_normalizationOverridesPending();

    void sequence_typicalWorkflow();
    void sequence_multiplePendingUpdates();
    void sequence_importOnlyWithPending();
};

void TestAdifModeSubMode::createModePair_initialState_data() {
    QTest::addColumn<QString>("mode");
    QTest::addColumn<QString>("submode");
    QTest::addColumn<QString>("expectedMode");
    QTest::addColumn<QString>("expectedSubmode");

    QTest::newRow("CW empty") << "CW"
                              << ""
                              << "CW"
                              << "";
    QTest::newRow("SSB+USB") << "SSB"
                             << "USB"
                             << "SSB"
                             << "USB";
    QTest::newRow("MFSK+FT4") << "MFSK"
                              << "FT4"
                              << "MFSK"
                              << "FT4";
}

void TestAdifModeSubMode::createModePair_initialState() {
    QFETCH(QString, mode);
    QFETCH(QString, submode);
    QFETCH(QString, expectedMode);
    QFETCH(QString, expectedSubmode);

    auto [modePtr, submodePtr] =
        AdifModeFactory::createModePair(mode.toStdString(), submode.toStdString());

    QVERIFY(modePtr != nullptr);
    QVERIFY(submodePtr != nullptr);
    QCOMPARE(modePtr->get(), expectedMode.toStdString());
    QCOMPARE(submodePtr->get(), expectedSubmode.toStdString());
}

void TestAdifModeSubMode::createModePair_normalization_data() {
    QTest::addColumn<QString>("mode");
    QTest::addColumn<QString>("submode");
    QTest::addColumn<QString>("expectedMode");
    QTest::addColumn<QString>("expectedSubmode");

    QTest::newRow("C4FM only -> DIGITALVOICE/C4FM") << "C4FM"
                                                    << ""
                                                    << "DIGITALVOICE"
                                                    << "C4FM";
    QTest::newRow("PSK31 only -> PSK/PSK31") << "PSK31"
                                             << ""
                                             << "PSK"
                                             << "PSK31";
    QTest::newRow("submode dictates mode") << "SSB"
                                           << "C4FM"
                                           << "DIGITALVOICE"
                                           << "C4FM";
    QTest::newRow("submode corrects mode") << "FM"
                                           << "DSTAR"
                                           << "DIGITALVOICE"
                                           << "DSTAR";
}

void TestAdifModeSubMode::createModePair_normalization() {
    QFETCH(QString, mode);
    QFETCH(QString, submode);
    QFETCH(QString, expectedMode);
    QFETCH(QString, expectedSubmode);

    auto [modePtr, submodePtr] =
        AdifModeFactory::createModePair(mode.toStdString(), submode.toStdString());

    QVERIFY(modePtr != nullptr);
    QVERIFY(submodePtr != nullptr);
    QCOMPARE(modePtr->get(), expectedMode.toStdString());
    QCOMPARE(submodePtr->get(), expectedSubmode.toStdString());
}

void TestAdifModeSubMode::createModePair_invalid_data() {
    QTest::addColumn<QString>("mode");
    QTest::addColumn<QString>("submode");

    QTest::newRow("invalid mode") << "INVALID"
                                  << "";
    QTest::newRow("invalid submode only") << "CW"
                                          << "INVALID_SUB";
    QTest::newRow("both invalid") << "ABC"
                                  << "XYZ";
}

void TestAdifModeSubMode::createModePair_invalid() {
    QFETCH(QString, mode);
    QFETCH(QString, submode);

    auto [modePtr, submodePtr] =
        AdifModeFactory::createModePair(mode.toStdString(), submode.toStdString());

    QVERIFY(modePtr != nullptr);
    QVERIFY(submodePtr != nullptr);
    QCOMPARE(modePtr->get(), std::string(""));
    QCOMPARE(submodePtr->get(), std::string(""));
}

void TestAdifModeSubMode::submode_set_rejectAndStorePending() {
    auto [modePtr, submodePtr] = AdifModeFactory::createModePair("CW", "");
    QVERIFY(modePtr != nullptr && submodePtr != nullptr);

    bool success = submodePtr->set("FT4");
    QVERIFY(!success);
    QCOMPARE(submodePtr->get(), std::string(""));

    modePtr->set("MFSK");
    QCOMPARE(modePtr->get(), std::string("MFSK"));
    QCOMPARE(submodePtr->get(), std::string("FT4"));
}

void TestAdifModeSubMode::submode_set_acceptAndClearPending() {
    auto [modePtr, submodePtr] = AdifModeFactory::createModePair("SSB", "USB");
    QVERIFY(modePtr != nullptr && submodePtr != nullptr);

    submodePtr->set("FT4");
    QCOMPARE(submodePtr->get(), std::string("USB"));

    bool success = submodePtr->set("LSB");
    QVERIFY(success);
    QCOMPARE(submodePtr->get(), std::string("LSB"));

    modePtr->set("SSB");
    QCOMPARE(submodePtr->get(), std::string("LSB"));
}

void TestAdifModeSubMode::submode_set_clearBoth() {
    auto [modePtr, submodePtr] = AdifModeFactory::createModePair("MFSK", "FT4");
    QVERIFY(modePtr != nullptr && submodePtr != nullptr);

    submodePtr->set("USB");
    QCOMPARE(submodePtr->get(), std::string("FT4"));

    bool success = submodePtr->set("");
    QVERIFY(success);
    QCOMPARE(submodePtr->get(), std::string(""));

    modePtr->set("SSB");
    QCOMPARE(submodePtr->get(), std::string(""));
}

void TestAdifModeSubMode::mode_set_activatesPendingSubmode() {
    auto [modePtr, submodePtr] = AdifModeFactory::createModePair("CW", "");
    QVERIFY(modePtr != nullptr && submodePtr != nullptr);

    submodePtr->set("FT4");
    QCOMPARE(submodePtr->get(), std::string(""));

    bool success = modePtr->set("MFSK");
    QVERIFY(success);
    QCOMPARE(modePtr->get(), std::string("MFSK"));
    QCOMPARE(submodePtr->get(), std::string("FT4"));
}

void TestAdifModeSubMode::mode_set_clearsIncompatibleSubmode() {
    auto [modePtr, submodePtr] = AdifModeFactory::createModePair("MFSK", "FT4");
    QVERIFY(modePtr != nullptr && submodePtr != nullptr);

    bool success = modePtr->set("CW");
    QVERIFY(success);
    QCOMPARE(modePtr->get(), std::string("CW"));
    QCOMPARE(submodePtr->get(), std::string(""));

    modePtr->set("MFSK");
    QCOMPARE(submodePtr->get(), std::string(""));
}

void TestAdifModeSubMode::mode_set_normalizationOverridesPending() {
    auto [modePtr, submodePtr] = AdifModeFactory::createModePair("CW", "");
    QVERIFY(modePtr != nullptr && submodePtr != nullptr);

    submodePtr->set("FT4");
    QCOMPARE(submodePtr->get(), std::string(""));

    bool success = modePtr->set("C4FM");
    QVERIFY(success);

    QCOMPARE(modePtr->get(), std::string("DIGITALVOICE"));
    QCOMPARE(submodePtr->get(), std::string("C4FM"));

    modePtr->set("MFSK");
    QCOMPARE(submodePtr->get(), std::string(""));
}

void TestAdifModeSubMode::sequence_typicalWorkflow() {

    auto [modePtr, submodePtr] = AdifModeFactory::createModePair("CW", "");
    QVERIFY(modePtr && submodePtr);
    QCOMPARE(modePtr->get(), std::string("CW"));
    QCOMPARE(submodePtr->get(), std::string(""));

    bool ret = submodePtr->set("FT4");
    QVERIFY(!ret);
    QCOMPARE(submodePtr->get(), std::string(""));

    ret = modePtr->set("MFSK");
    QVERIFY(ret);
    QCOMPARE(modePtr->get(), std::string("MFSK"));
    QCOMPARE(submodePtr->get(), std::string("FT4"));

    ret = submodePtr->set("USB");
    QVERIFY(!ret);
    QCOMPARE(submodePtr->get(), std::string("FT4"));

    ret = modePtr->set("FT8");
    QVERIFY(ret);
    QCOMPARE(modePtr->get(), std::string("FT8"));
    QCOMPARE(submodePtr->get(), std::string(""));
}

void TestAdifModeSubMode::sequence_multiplePendingUpdates() {
    auto [modePtr, submodePtr] = AdifModeFactory::createModePair("CW", "");
    QVERIFY(modePtr && submodePtr);

    submodePtr->set("FT4");
    submodePtr->set("USB");
    submodePtr->set("PSK31");

    modePtr->set("PSK");
    QCOMPARE(modePtr->get(), std::string("PSK"));
    QCOMPARE(submodePtr->get(), std::string("PSK31"));
}

void TestAdifModeSubMode::sequence_importOnlyWithPending() {
    auto [modePtr, submodePtr] = AdifModeFactory::createModePair("CW", "");
    QVERIFY(modePtr && submodePtr);

    submodePtr->set("FT4");
    QCOMPARE(submodePtr->get(), std::string(""));

    modePtr->set("PSK31");
    QCOMPARE(modePtr->get(), std::string("PSK"));
    QCOMPARE(submodePtr->get(), std::string("PSK31"));

    modePtr->set("MFSK");
    QCOMPARE(submodePtr->get(), std::string(""));
}

QTEST_APPLESS_MAIN(TestAdifModeSubMode)
#include "tst_adifmodesubmode.moc"