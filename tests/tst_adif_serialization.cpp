#include <QtTest>
#include <QString>
#include <QTextStream>

#include "adif_serialization.h"
#include "record.h"

class AdifSerializationTest : public QObject {
    Q_OBJECT

  private slots:
    void test_toAdif_roundTrip();
    void test_toCsv_row();
    void test_toAdif_specialCharacters();
};

void AdifSerializationTest::test_toAdif_roundTrip() {
    GRecord r;
    QVERIFY(r.addOrSetPair("call", "W1AW"));
    QVERIFY(r.addOrSetPair("notes", "line1\nline2"));

    QString storage;
    QTextStream stream(&storage);
    AdifSerialization::toAdif(stream, std::vector<GRecord>{r});
    QVERIFY(storage.contains(QLatin1String("<call:")));
    QVERIFY(storage.contains(QLatin1String("<eor>")));
}

void AdifSerializationTest::test_toCsv_row() {
    GRecord r;
    QVERIFY(r.addOrSetPair("call", "W2BB"));
    QVERIFY(r.addOrSetPair("notes", "a,b\"c"));

    QString out;
    QTextStream stream(&out);
    std::vector<std::string> headers = {"call", "notes"};
    AdifSerialization::toCsv(stream, std::vector<GRecord>{r}, headers);
    QVERIFY(out.contains(QLatin1String("call,notes")));
    QVERIFY(out.contains(QLatin1String("W2BB")));
}

void AdifSerializationTest::test_toAdif_specialCharacters() {
    GRecord r;
    QVERIFY(r.addOrSetPair("call", "W3CC"));
    QVERIFY(r.addOrSetPair("notes", "has<angle"));

    QString storage;
    QTextStream stream(&storage);
    AdifSerialization::toAdif(stream, std::vector<GRecord>{r});
    QVERIFY(!storage.isEmpty());
}

QTEST_MAIN(AdifSerializationTest)
#include "tst_adif_serialization.moc"
