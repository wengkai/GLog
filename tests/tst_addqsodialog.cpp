#include <QtTest>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>

#include "AddQSODialog.h"
#include "adifdb.h"

class AddQSODialogTest : public QObject {
    Q_OBJECT

  private slots:
    void test_accept_empty_required_field_emits_information();
    void test_accept_invalid_call_emits_warning();
    void test_accept_valid_inserts_row_no_warning();
};

static QPushButton *okButton(AddQSODialog &dlg) {
    auto *box = dlg.findChild<QDialogButtonBox *>(QStringLiteral("buttonBox"));
    if (!box) {
        return nullptr;
    }
    return qobject_cast<QPushButton *>(box->button(QDialogButtonBox::Ok));
}

void AddQSODialogTest::test_accept_empty_required_field_emits_information() {
    AdifModel model;
    QCoreApplication::processEvents();
    QCOMPARE(model.rowCount(), 0);

    AddQSODialog dlg(&model);
    dlg.show();
    QCoreApplication::processEvents();

    auto *call = dlg.findChild<QLineEdit *>(QStringLiteral("callSignLineEdit"));
    QVERIFY(call);
    call->clear();

    QSignalSpy infoSpy(&dlg, &AddQSODialog::userInformation);
    QSignalSpy warnSpy(&dlg, &AddQSODialog::userWarning);

    auto *ok = okButton(dlg);
    QVERIFY(ok);
    QTest::mouseClick(ok, Qt::LeftButton);
    QCoreApplication::processEvents();

    QCOMPARE(infoSpy.count(), 1);
    QCOMPARE(warnSpy.count(), 0);
    QCOMPARE(model.rowCount(), 0);
    QVERIFY(dlg.result() != QDialog::Accepted);
}

void AddQSODialogTest::test_accept_invalid_call_emits_warning() {
    AdifModel model;
    QCoreApplication::processEvents();

    AddQSODialog dlg(&model);
    dlg.show();
    QCoreApplication::processEvents();

    auto *call = dlg.findChild<QLineEdit *>(QStringLiteral("callSignLineEdit"));
    auto *rst = dlg.findChild<QLineEdit *>(QStringLiteral("rstLineEdit"));
    auto *rstR = dlg.findChild<QLineEdit *>(QStringLiteral("rstRcvdLineEdit"));
    auto *freq = dlg.findChild<QDoubleSpinBox *>(QStringLiteral("freqDoubleSpinBox"));
    auto *mode = dlg.findChild<QComboBox *>(QStringLiteral("modeComboBox"));
    QVERIFY(call && rst && rstR && freq && mode);
    QVERIFY(mode->count() > 0);

    call->setText(QStringLiteral("bad call!"));
    rst->setText(QStringLiteral("59"));
    rstR->setText(QStringLiteral("59"));
    freq->setValue(14.074);
    mode->setCurrentIndex(0);

    QSignalSpy infoSpy(&dlg, &AddQSODialog::userInformation);
    QSignalSpy warnSpy(&dlg, &AddQSODialog::userWarning);

    auto *ok = okButton(dlg);
    QVERIFY(ok);
    QTest::mouseClick(ok, Qt::LeftButton);
    QCoreApplication::processEvents();

    QCOMPARE(warnSpy.count(), 1);
    QCOMPARE(infoSpy.count(), 0);
    QCOMPARE(model.rowCount(), 0);
}

void AddQSODialogTest::test_accept_valid_inserts_row_no_warning() {
    AdifModel model;
    QCoreApplication::processEvents();
    QCOMPARE(model.rowCount(), 0);

    AddQSODialog dlg(&model);
    dlg.show();
    QCoreApplication::processEvents();

    auto *call = dlg.findChild<QLineEdit *>(QStringLiteral("callSignLineEdit"));
    auto *rst = dlg.findChild<QLineEdit *>(QStringLiteral("rstLineEdit"));
    auto *rstR = dlg.findChild<QLineEdit *>(QStringLiteral("rstRcvdLineEdit"));
    auto *freq = dlg.findChild<QDoubleSpinBox *>(QStringLiteral("freqDoubleSpinBox"));
    auto *mode = dlg.findChild<QComboBox *>(QStringLiteral("modeComboBox"));
    QVERIFY(call && rst && rstR && freq && mode);
    QVERIFY(mode->count() > 0);

    call->setText(QStringLiteral("w1aw"));
    rst->setText(QStringLiteral("59"));
    rstR->setText(QStringLiteral("57"));
    freq->setValue(7.04);
    mode->setCurrentIndex(0);

    QSignalSpy infoSpy(&dlg, &AddQSODialog::userInformation);
    QSignalSpy warnSpy(&dlg, &AddQSODialog::userWarning);

    auto *ok = okButton(dlg);
    QVERIFY(ok);
    QTest::mouseClick(ok, Qt::LeftButton);
    QCoreApplication::processEvents();

    QCOMPARE(infoSpy.count(), 0);
    QCOMPARE(warnSpy.count(), 0);
    QCOMPARE(model.rowCount(), 1);

    bool saw_call = false;
    const int cols = model.columnCount();
    for (int c = 0; c < cols; ++c) {
        const QModelIndex ix = model.index(0, c);
        if (ix.data(Qt::DisplayRole)
                .toString()
                .compare(QLatin1String("W1AW"), Qt::CaseInsensitive) == 0) {
            saw_call = true;
            break;
        }
    }
    QVERIFY(saw_call);
    QCOMPARE(dlg.result(), int(QDialog::Accepted));
}

QTEST_MAIN(AddQSODialogTest)
#include "tst_addqsodialog.moc"
