#include <QtTest>
#include <QCoreApplication>
#include <vector>

#include "GLogApplication.h"
#include "adifdb.h"
#include "duplicatesmodel.h"
#include "record.h"

class DuplicatesModelTest : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase();
    void test_build_rowCount_data_roles();
    void test_setMajor_collectMinors();

  private:
    AdifModel m_model;
    DuplicatesModel m_dup{&m_model};
};

void DuplicatesModelTest::initTestCase() {
    (void)GLogApplication::getCtyDBInstance()->loadLocalDB();
}

void DuplicatesModelTest::test_build_rowCount_data_roles() {
    std::vector<GRecord> recs;
    for (int i = 0; i < 2; ++i) {
        GRecord r;
        QVERIFY(r.addOrSetPair("call", "SAMECALL"));
        recs.push_back(std::move(r));
    }
    m_model.insertRecords(0, std::move(recs));
    QCoreApplication::processEvents();

    const std::vector<std::string> fields = {"call"};
    auto clusters = m_model.findDuplicates(fields);
    QCOMPARE(clusters.size(), size_t(1));
    QCOMPARE(clusters[0].size(), size_t(2));

    m_dup.build(clusters);
    QCOMPARE(m_dup.rowCount({}), 1);
    const QModelIndex g0 = m_dup.index(0, 0, {});
    QCOMPARE(m_dup.rowCount(g0), 2);
    QVERIFY(!m_dup.data(g0, Qt::DisplayRole).toString().isEmpty());

    const QModelIndex c0 = m_dup.index(0, 0, g0);
    QVERIFY(m_dup.data(c0, Qt::DisplayRole).toString().contains(QLatin1String("SAMECALL")));
    QVERIFY(m_dup.data(c0, Qt::BackgroundRole).isValid());

    m_dup.setGroupStatus(0, DuplicateNode::GroupStatus::Success);
    QVERIFY(m_dup.data(g0, Qt::BackgroundRole).isValid());
}

void DuplicatesModelTest::test_setMajor_collectMinors() {
    m_model.clear();
    QCoreApplication::processEvents();

    std::vector<GRecord> recs;
    for (int i = 0; i < 2; ++i) {
        GRecord r;
        QVERIFY(r.addOrSetPair("call", "DUP2"));
        recs.push_back(std::move(r));
    }
    m_model.insertRecords(0, std::move(recs));
    QCoreApplication::processEvents();

    auto clusters = m_model.findDuplicates(std::vector<std::string>{"call"});
    m_dup.build(clusters);

    const QModelIndex g0 = m_dup.index(0, 0, {});
    const QModelIndex first = m_dup.index(0, 0, g0);
    const QModelIndex second = m_dup.index(1, 0, g0);
    QVERIFY(first.data(Qt::DisplayRole).toString().length() > 0);

    m_dup.setMajor(second);
    const auto minors = m_dup.collectMinors();
    QCOMPARE(minors.size(), 1);
    QCOMPARE(minors[0].row(), first.row());

    const auto groups = m_dup.collectGroups();
    QCOMPARE(groups.size(), size_t(1));
    QVERIFY(groups[0].major.isValid());
    QCOMPARE(groups[0].minors.size(), 1);
}

QTEST_MAIN(DuplicatesModelTest)
#include "tst_duplicatesmodel.moc"
