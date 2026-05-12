#include <QtTest>

#include <QCoreApplication>
#include <QLineEdit>
#include <QTreeView>

#include "DuplicatesManager.h"
#include "GLogApplication.h"
#include "adifdb.h"
#include "duplicatesmodel.h"
#include "record.h"

class DuplicatesManagerTest : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase();
    void test_refresh_duplicates_populates_tree();
    void test_remove_minors_after_confirmed_async();
    void test_merge_to_major_after_confirmed_async();

  private:
    static void drainUi() {
        for (int i = 0; i < 80; ++i) {
            QCoreApplication::processEvents();
        }
    }
};

void DuplicatesManagerTest::initTestCase() {
    (void)GLogApplication::getCtyDBInstance()->loadLocalDB();
}

void DuplicatesManagerTest::test_refresh_duplicates_populates_tree() {
    AdifModel model;
    std::vector<GRecord> recs;
    for (int i = 0; i < 2; ++i) {
        GRecord r;
        QVERIFY(r.addOrSetPair("call", "DMGRDUP"));
        recs.push_back(std::move(r));
    }
    model.insertRecords(0, std::move(recs));
    drainUi();

    DuplicatesManager dlg(&model);
    dlg.show();
    drainUi();

    auto *fields = dlg.findChild<QLineEdit *>(QStringLiteral("fieldsEdit"));
    QVERIFY(fields);
    fields->setText(QStringLiteral("call"));

    dlg.RefreshDuplicates();
    drainUi();

    auto *tv = dlg.findChild<QTreeView *>(QStringLiteral("treeView"));
    QVERIFY(tv);
    auto *dupModel = dynamic_cast<DuplicatesModel *>(tv->model());
    QVERIFY(dupModel);
    QCOMPARE(dupModel->rowCount({}), 1);
    const QModelIndex g0 = dupModel->index(0, 0, {});
    QCOMPARE(dupModel->rowCount(g0), 2);
}

void DuplicatesManagerTest::test_remove_minors_after_confirmed_async() {
    AdifModel model;
    std::vector<GRecord> recs;
    for (int i = 0; i < 2; ++i) {
        GRecord r;
        QVERIFY(r.addOrSetPair("call", "DMGRRM"));
        recs.push_back(std::move(r));
    }
    model.insertRecords(0, std::move(recs));
    drainUi();

    DuplicatesManager dlg(&model);
    dlg.show();
    drainUi();

    auto *fieldsRm = dlg.findChild<QLineEdit *>(QStringLiteral("fieldsEdit"));
    QVERIFY(fieldsRm);
    fieldsRm->setText(QStringLiteral("call"));
    dlg.RefreshDuplicates();
    drainUi();

    auto *tv = dlg.findChild<QTreeView *>(QStringLiteral("treeView"));
    QVERIFY(tv);
    auto *dupModel = dynamic_cast<DuplicatesModel *>(tv->model());
    QVERIFY(dupModel);
    const QModelIndex g0 = dupModel->index(0, 0, {});
    const QModelIndex second = dupModel->index(1, 0, g0);
    dupModel->setMajor(second);
    drainUi();

    QSignalSpy spy(&dlg, &DuplicatesManager::userInformation);
    QVERIFY(spy.isValid());
    dlg.removeMinorsAfterConfirmed();
    while (spy.count() == 0) {
        QCoreApplication::processEvents();
    }

    QTRY_COMPARE(model.rowCount(), 1);
    drainUi();
    dlg.RefreshDuplicates();
    drainUi();
    QCOMPARE(dupModel->rowCount({}), 0);
}

void DuplicatesManagerTest::test_merge_to_major_after_confirmed_async() {
    AdifModel model;
    std::vector<GRecord> recs;
    for (int i = 0; i < 2; ++i) {
        GRecord r;
        QVERIFY(r.addOrSetPair("call", "DMGRMG"));
        if (i == 1) {
            QVERIFY(r.addOrSetPair("notes", QStringLiteral("minor-note").toStdString()));
        }
        recs.push_back(std::move(r));
    }
    model.insertRecords(0, std::move(recs));
    drainUi();

    DuplicatesManager dlg(&model);
    dlg.show();
    drainUi();

    auto *fieldsMg = dlg.findChild<QLineEdit *>(QStringLiteral("fieldsEdit"));
    QVERIFY(fieldsMg);
    fieldsMg->setText(QStringLiteral("call"));
    dlg.RefreshDuplicates();
    drainUi();

    auto *tv = dlg.findChild<QTreeView *>(QStringLiteral("treeView"));
    QVERIFY(tv);
    auto *dupModel = dynamic_cast<DuplicatesModel *>(tv->model());
    QVERIFY(dupModel);
    const QModelIndex g0 = dupModel->index(0, 0, {});
    const QModelIndex major = dupModel->index(0, 0, g0);
    dupModel->setMajor(major);
    drainUi();

    QSignalSpy spy(&dlg, &DuplicatesManager::userInformation);
    QVERIFY(spy.isValid());
    dlg.mergeToMajorAfterConfirmed();
    while (spy.count() == 0) {
        QCoreApplication::processEvents();
    }
    model.getFIFOBackendThreadExecutor()->waitForDone();
    drainUi();
    QCOMPARE(model.rowCount(), 2);
    drainUi();
}

QTEST_MAIN(DuplicatesManagerTest)
#include "tst_duplicatesmanager.moc"
