#include <QtTest>
#include <QCoreApplication>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "record.h"

class GRecordTest : public QObject {
    Q_OBJECT

  private slots:
    void test_clone_roundTrip();
    void test_cloneForPersistence_and_cloneForSyncOrder();
    void test_addOrSetPair_invalidValue_noCorruption();
    void test_tryRemoveField_and_findItem();
    void test_remap();
    void test_merge_addsNonConflictingFields();
    void test_merge_rejectsConflict();
    void test_less_and_equal_to();
    void test_writeTo_includesEor();
    void test_getValueByField_paths();
};

void GRecordTest::test_clone_roundTrip() {
    GRecord a;
    QVERIFY(a.addOrSetPair("call", "W1AW"));
    QVERIFY(a.addOrSetPair("notes", "hello"));

    GRecord b = a.clone();
    const std::vector<std::string> fields = {"call", "notes", "mode", "band", "freq"};
    QVERIFY(GRecord::equal_to(a, b, fields));
}

void GRecordTest::test_cloneForPersistence_and_cloneForSyncOrder() {
    GRecord a;
    QVERIFY(a.addOrSetPair("call", "K1A"));
    a.setDbId(42);

    GRecord p = a.cloneForPersistence();
    QCOMPARE(p.getDbId(), a.getDbId());
    QVERIFY(a.at("call") == p.at("call"));

    GRecord s = a.cloneForSyncOrder();
    QCOMPARE(s.getDbId(), a.getDbId());
    QVERIFY(s.find("call") == s.end());
}

void GRecordTest::test_addOrSetPair_invalidValue_noCorruption() {
    GRecord r;
    QVERIFY(r.find("notes") == r.end());
    QVERIFY(!r.addOrSetPair("qso_date", "not-a-real-adif-date-xyz"));
    QVERIFY(r.find("notes") == r.end());
}

void GRecordTest::test_tryRemoveField_and_findItem() {
    GRecord r;
    QVERIFY(r.addOrSetPair("notes", "x"));
    QVERIFY(r.findItem("notes"));
    QCOMPARE(static_cast<std::string>(r.at("notes")), std::string("x"));
    QVERIFY(!r.tryRemoveField("mode"));
    QVERIFY(r.tryRemoveField("notes"));
    QVERIFY(!r.findItem("notes"));
}

void GRecordTest::test_remap() {
    GRecord r;
    QVERIFY(r.addOrSetPair("call", "W2BB"));
    GRecord m = r.remap();
    QVERIFY(GRecord::equal_to(r, m, std::vector<std::string>{"call"}));
}

void GRecordTest::test_merge_addsNonConflictingFields() {
    GRecord major;
    QVERIFY(major.addOrSetPair("call", "W3CC"));
    GRecord extra;
    QVERIFY(extra.addOrSetPair("call", "W3CC"));
    QVERIFY(extra.addOrSetPair("notes", "merged-note"));

    QVERIFY(major.merge(std::vector<GRecord>{extra}));
    QVERIFY(major.find("notes") != major.end());
    QCOMPARE(static_cast<std::string>(major.at("notes")), std::string("merged-note"));
}

void GRecordTest::test_merge_rejectsConflict() {
    GRecord major;
    QVERIFY(major.addOrSetPair("call", "W4DD"));
    GRecord other;
    QVERIFY(other.addOrSetPair("call", "W5EE"));

    QVERIFY(!major.merge(std::vector<GRecord>{other}));
}

void GRecordTest::test_less_and_equal_to() {
    GRecord a;
    GRecord b;
    QVERIFY(a.addOrSetPair("call", "AAA"));
    QVERIFY(b.addOrSetPair("call", "BBB"));
    const std::vector<std::string> byCall = {"call"};
    QVERIFY(GRecord::less(a, b, byCall));
    QVERIFY(!GRecord::equal_to(a, b, byCall));
    QVERIFY(GRecord::equal_to(a, a, byCall));
}

void GRecordTest::test_writeTo_includesEor() {
    GRecord r;
    QVERIFY(r.addOrSetPair("call", "W6FF"));
    std::ostringstream oss;
    r.writeTo(oss);
    const std::string s = oss.str();
    QVERIFY(s.find("<call:") != std::string::npos);
    QVERIFY(s.find("<eor>") != std::string::npos);
}

void GRecordTest::test_getValueByField_paths() {
    GRecord r;
    QVERIFY(r.addOrSetPair("call", "W7GG"));

    uint64_t len = 0;
    QCOMPARE(GRecord::getValueByField(nullptr, "call", 4, nullptr, &len, 0),
             IGRecord::Result::InvalidInput);

    QCOMPARE(GRecord::getValueByField(&r, "call", 4, nullptr, nullptr, 0),
             IGRecord::Result::InvalidResultLenOutput);

    len = 0;
    QCOMPARE(GRecord::getValueByField(&r, "nosuch", 6, nullptr, &len, 0),
             IGRecord::Result::NotFound);

    len = 0;
    QCOMPARE(GRecord::getValueByField(&r, "call", 4, nullptr, &len, 0), IGRecord::Result::Overflow);
    QVERIFY(len >= 4);

    std::vector<char> buf(len + 2, '\0');
    uint64_t out = 0;
    QCOMPARE(GRecord::getValueByField(&r, "call", 4, buf.data(), &out, buf.size()),
             IGRecord::Result::NoError);
    QCOMPARE(out, static_cast<uint64_t>(4));
    QCOMPARE(std::string(buf.data(), buf.data() + out), std::string("W7GG"));
}

QTEST_MAIN(GRecordTest)
#include "tst_record.moc"
