#include <QtTest>

#include <QCoreApplication>
#include <QItemSelectionModel>
#include <QMimeData>
#include <QSignalSpy>
#include <QThreadPool>
#include <QWidget>
#include <set>

#include "DropAbleTableView.h"
#include "adifdb.h"
#include "record.h"

class DropAbleTableViewTest : public QObject {
    Q_OBJECT

  private slots:
    void test_construct_header_and_model();
    void test_findNext_empty_model_emits_selected_only();
    void test_findNext_no_current_emits_findNextSignal_with_invalid_index();
    void test_findNext_empty_key_advances_column();
    void test_foundNext_valid_index_updates_current();
    void test_foundNext_invalid_emits_userInformation_and_selected();
    void test_proceedDeleteSelectedRows_removes_row();
    void test_tryCopySelectedRows_emits_setClipboard();
    void test_selectRows_selects_expected_rows();
    void test_keyPress_non_delete_does_not_remove_rows();
    void test_proceedPasteRows_empty_mime_no_crash();

  private:
    static void drainUi() {
        QThreadPool::globalInstance()->waitForDone();
        for (int i = 0; i < 80; ++i) {
            QCoreApplication::processEvents();
        }
    }

    static std::vector<GRecord> makeRecords(int count, const char *callPrefix) {
        std::vector<GRecord> recs;
        recs.reserve(static_cast<size_t>(count));
        for (int i = 0; i < count; ++i) {
            GRecord r;
            const QString call = QStringLiteral("%1%2").arg(QLatin1String(callPrefix)).arg(i);
            if (!r.addOrSetPair("call", call.toStdString()) || !r.addOrSetPair("mode", "CW")) {
                return {};
            }
            recs.push_back(std::move(r));
        }
        return recs;
    }
};

void DropAbleTableViewTest::test_construct_header_and_model() {
    AdifModel model;
    DropAbleTableView view(&model);
    QVERIFY(view.getHeaderView() != nullptr);
    QCOMPARE(view.model(), &model);
}

void DropAbleTableViewTest::test_findNext_empty_model_emits_selected_only() {
    AdifModel model;
    DropAbleTableView view(&model);
    view.show();
    QCoreApplication::processEvents();

    QSignalSpy selectedSpy(&view, &DropAbleTableView::selected);
    QSignalSpy findSpy(&view, &DropAbleTableView::findNextSignal);

    view.findNext(QStringLiteral("call"), QStringLiteral("W1"), false);
    QCoreApplication::processEvents();

    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(selectedSpy.count(), 1);
    QCOMPARE(findSpy.count(), 0);
}

void DropAbleTableViewTest::test_findNext_no_current_emits_findNextSignal_with_invalid_index() {
    AdifModel model;
    auto recs = makeRecords(1, "DMGTV");
    QCOMPARE(recs.size(), 1U);
    model.insertRecords(0, std::move(recs));
    drainUi();

    DropAbleTableView view(&model);
    view.show();
    drainUi();

    view.selectionModel()->clearCurrentIndex();
    QVERIFY(!view.selectionModel()->currentIndex().isValid());

    QSignalSpy findSpy(&view, &DropAbleTableView::findNextSignal);
    view.findNext(QStringLiteral("call"), QStringLiteral("DM"), false);
    QCoreApplication::processEvents();

    QCOMPARE(findSpy.count(), 1);
    const auto args = findSpy.takeFirst();
    QVERIFY(!args.at(0).value<QModelIndex>().isValid());
}

void DropAbleTableViewTest::test_findNext_empty_key_advances_column() {
    AdifModel model;
    auto recs = makeRecords(1, "DMGCL");
    QCOMPARE(recs.size(), 1U);
    model.insertRecords(0, std::move(recs));
    drainUi();

    QVERIFY(model.columnCount() >= 2);

    DropAbleTableView view(&model);
    view.show();
    drainUi();

    const QModelIndex topLeft = model.index(0, 0);
    view.setCurrentIndex(topLeft);
    QCOMPARE(view.selectionModel()->currentIndex(), topLeft);

    QSignalSpy findSpy(&view, &DropAbleTableView::findNextSignal);
    view.findNext(QString(), QString(), false);
    QCoreApplication::processEvents();

    QCOMPARE(findSpy.count(), 1);
    const QModelIndex next = findSpy.at(0).at(0).value<QModelIndex>();
    QVERIFY(next.isValid());
    QCOMPARE(next.row(), 0);
    QCOMPARE(next.column(), 1);
}

void DropAbleTableViewTest::test_foundNext_valid_index_updates_current() {
    AdifModel model;
    auto recs = makeRecords(2, "DMGFV");
    QCOMPARE(recs.size(), 2U);
    model.insertRecords(0, std::move(recs));
    drainUi();

    DropAbleTableView view(&model);
    view.show();
    drainUi();

    const QModelIndex target = model.index(1, 0);
    QVERIFY(target.isValid());

    QSignalSpy selectedSpy(&view, &DropAbleTableView::selected);
    view.foundNext(target);
    QCoreApplication::processEvents();

    QCOMPARE(view.selectionModel()->currentIndex(), target);
    QCOMPARE(selectedSpy.count(), 1);
}

void DropAbleTableViewTest::test_foundNext_invalid_emits_userInformation_and_selected() {
    AdifModel model;
    auto recs = makeRecords(1, "DMGIF");
    QCOMPARE(recs.size(), 1U);
    model.insertRecords(0, std::move(recs));
    drainUi();

    DropAbleTableView view(&model);
    view.show();
    drainUi();

    QSignalSpy infoSpy(&view, &DropAbleTableView::userInformation);
    QSignalSpy selectedSpy(&view, &DropAbleTableView::selected);

    view.foundNext(QModelIndex());
    QCoreApplication::processEvents();

    QCOMPARE(infoSpy.count(), 1);
    QCOMPARE(selectedSpy.count(), 1);
}

void DropAbleTableViewTest::test_proceedDeleteSelectedRows_removes_row() {
    AdifModel model;
    auto recs = makeRecords(2, "DMGDL");
    QCOMPARE(recs.size(), 2U);
    model.insertRecords(0, std::move(recs));
    drainUi();

    QCOMPARE(model.rowCount(), 2);

    DropAbleTableView view(&model);
    view.show();
    drainUi();

    view.selectionModel()->select(model.index(0, 0),
                                  QItemSelectionModel::Rows | QItemSelectionModel::Select);
    const QModelIndexList rows = view.selectionModel()->selectedRows();
    QVERIFY(!rows.isEmpty());

    view.proceedDeleteSelectedRows(rows);
    drainUi();

    QCOMPARE(model.rowCount(), 1);
}

void DropAbleTableViewTest::test_tryCopySelectedRows_emits_setClipboard() {
    AdifModel model;
    auto recs = makeRecords(1, "DMGCP");
    QCOMPARE(recs.size(), 1U);
    model.insertRecords(0, std::move(recs));
    drainUi();

    DropAbleTableView view(&model);
    view.show();
    drainUi();

    view.selectionModel()->select(model.index(0, 0),
                                  QItemSelectionModel::Rows | QItemSelectionModel::Select);

    QSignalSpy clipSpy(&model, &AdifModel::setCilpboard);
    view.tryCopySelectedRows();
    QCoreApplication::processEvents();

    QCOMPARE(clipSpy.count(), 1);
}

void DropAbleTableViewTest::test_selectRows_selects_expected_rows() {
    AdifModel model;
    auto recs = makeRecords(3, "DMGSR");
    QCOMPARE(recs.size(), 3U);
    model.insertRecords(0, std::move(recs));
    drainUi();

    DropAbleTableView view(&model);
    view.show();
    drainUi();

    view.selectRows({0, 2});
    drainUi();

    const auto selected = view.selectionModel()->selectedRows();
    QCOMPARE(selected.size(), 2);
    std::set<int> rows;
    for (const QModelIndex &ix : selected) {
        rows.insert(ix.row());
    }
    QCOMPARE(rows, (std::set<int>{0, 2}));
}

void DropAbleTableViewTest::test_keyPress_non_delete_does_not_remove_rows() {
    AdifModel model;
    auto recs = makeRecords(2, "DMGKY");
    QCOMPARE(recs.size(), 2U);
    model.insertRecords(0, std::move(recs));
    drainUi();

    DropAbleTableView view(&model);
    view.setFocusPolicy(Qt::StrongFocus);
    view.show();
    view.raise();
    view.activateWindow();
    view.setFocus();
    drainUi();

    QCOMPARE(model.rowCount(), 2);
    QTest::keyClick(&view, Qt::Key_A);
    drainUi();

    QCOMPARE(model.rowCount(), 2);
}

void DropAbleTableViewTest::test_proceedPasteRows_empty_mime_no_crash() {
    AdifModel model;
    auto recs = makeRecords(1, "DMGPT");
    QCOMPARE(recs.size(), 1U);
    model.insertRecords(0, std::move(recs));
    drainUi();

    const int rowsBefore = model.rowCount();

    DropAbleTableView view(&model);
    view.show();
    drainUi();

    QMimeData empty;
    view.proceedPasteRows(&empty);
    drainUi();

    QCOMPARE(model.rowCount(), rowsBefore);
}

QTEST_MAIN(DropAbleTableViewTest)
#include "tst_dropabletableview.moc"
