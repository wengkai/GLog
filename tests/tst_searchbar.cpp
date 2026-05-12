#include <QtTest>

#include <QCheckBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalSpy>

#include "SearchBar.h"

class SearchBarTest : public QObject {
    Q_OBJECT

  private slots:
    void test_findNext_signal_and_buttons();
    void test_selectAll_signal_and_buttons();
    void test_deselectAll_signal_regex_toggle();
    void test_enableButtons_restores_state();
};

void SearchBarTest::test_findNext_signal_and_buttons() {
    SearchBar bar;
    bar.show();
    QCoreApplication::processEvents();

    auto *line = bar.findChild<QLineEdit *>(QStringLiteral("lineEdit"));
    auto *plain = bar.findChild<QPlainTextEdit *>(QStringLiteral("plainTextEdit"));
    auto *regex = bar.findChild<QCheckBox *>(QStringLiteral("regexModeCheckBox"));
    auto *findBtn = bar.findChild<QPushButton *>(QStringLiteral("findNextButton"));
    QVERIFY(line && plain && regex && findBtn);

    line->setText(QStringLiteral("CALL"));
    plain->setPlainText(QStringLiteral("value text"));
    regex->setChecked(false);

    QSignalSpy spy(&bar, &SearchBar::findNext);
    QTest::mouseClick(findBtn, Qt::LeftButton);
    QCOMPARE(spy.count(), 1);

    const auto args = spy.takeFirst();
    QCOMPARE(args.at(0).toString(), QStringLiteral("call"));
    QCOMPARE(args.at(1).toString(), QStringLiteral("value text"));
    QCOMPARE(args.at(2).toBool(), false);

    QVERIFY(!findBtn->isEnabled());
}

void SearchBarTest::test_selectAll_signal_and_buttons() {
    SearchBar bar;
    bar.show();
    QCoreApplication::processEvents();

    auto *line = bar.findChild<QLineEdit *>(QStringLiteral("lineEdit"));
    auto *plain = bar.findChild<QPlainTextEdit *>(QStringLiteral("plainTextEdit"));
    auto *selBtn = bar.findChild<QPushButton *>(QStringLiteral("selectAllButton"));
    auto *clearBtn = bar.findChild<QPushButton *>(QStringLiteral("clearAllButton"));
    QVERIFY(line && plain && selBtn && clearBtn);

    line->setText(QStringLiteral("Mode"));
    plain->setPlainText(QStringLiteral("cw"));

    QSignalSpy spy(&bar, &SearchBar::selectAll);
    QTest::mouseClick(selBtn, Qt::LeftButton);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).toString(), QStringLiteral("mode"));
    QCOMPARE(spy.first().at(1).toString(), QStringLiteral("cw"));

    QVERIFY(!selBtn->isEnabled());
    QVERIFY(!clearBtn->isEnabled());
}

void SearchBarTest::test_deselectAll_signal_regex_toggle() {
    SearchBar bar;
    bar.show();
    QCoreApplication::processEvents();

    auto *line = bar.findChild<QLineEdit *>(QStringLiteral("lineEdit"));
    auto *plain = bar.findChild<QPlainTextEdit *>(QStringLiteral("plainTextEdit"));
    auto *regex = bar.findChild<QCheckBox *>(QStringLiteral("regexModeCheckBox"));
    auto *clearBtn = bar.findChild<QPushButton *>(QStringLiteral("clearAllButton"));
    QVERIFY(line && plain && regex && clearBtn);

    line->setText(QStringLiteral("FREQ"));
    plain->setPlainText(QStringLiteral("14"));
    regex->setChecked(true);

    QSignalSpy spy(&bar, &SearchBar::deselectAll);
    QTest::mouseClick(clearBtn, Qt::LeftButton);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).toString(), QStringLiteral("freq"));
    QCOMPARE(spy.first().at(1).toString(), QStringLiteral("14"));
    QCOMPARE(spy.first().at(2).toBool(), true);
}

void SearchBarTest::test_enableButtons_restores_state() {
    SearchBar bar;
    bar.show();
    QCoreApplication::processEvents();

    auto *findBtn = bar.findChild<QPushButton *>(QStringLiteral("findNextButton"));
    auto *selBtn = bar.findChild<QPushButton *>(QStringLiteral("selectAllButton"));
    auto *clearBtn = bar.findChild<QPushButton *>(QStringLiteral("clearAllButton"));
    QVERIFY(findBtn && selBtn && clearBtn);

    QTest::mouseClick(findBtn, Qt::LeftButton);
    QVERIFY(!findBtn->isEnabled() && !selBtn->isEnabled() && !clearBtn->isEnabled());

    bar.enableButtons();
    QVERIFY(findBtn->isEnabled() && selBtn->isEnabled() && clearBtn->isEnabled());
}

QTEST_MAIN(SearchBarTest)
#include "tst_searchbar.moc"
