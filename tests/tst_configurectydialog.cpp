#include <QtTest>

#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QUrl>

#include "ConfigureCtyDialog.h"
#include "GLogApplication.h"
#include "ctydb.h"

class ConfigureCtyDialogTest : public QObject {
    Q_OBJECT

    GLogApplication *m_app = nullptr;

  private slots:
    void initTestCase();
    void cleanupTestCase();

    void test_success_msg_non_empty();
    void test_set_db_hint_updates_line_edit();
    void test_disable_enable_ok_button();
    void test_ok_loads_local_minimal_cty_without_browse();

  private:
    static void drainUi() {
        for (int i = 0; i < 120; ++i) {
            QCoreApplication::processEvents();
        }
    }

    static QString minimalCtyDat() {
        // One valid block for CtyDB::loadDBString:
        // name:cq:itu:continent:lat:lon:utc:p_prefix:patterns
        return QStringLiteral("TestEntity:1:1:NA:0:0:0:TT:TT;");
    }

    static QPushButton *okButton(ConfigureCtyDialog &dlg) {
        auto *box = dlg.findChild<QDialogButtonBox *>(QStringLiteral("buttonBox"));
        if (!box) {
            return nullptr;
        }
        return qobject_cast<QPushButton *>(box->button(QDialogButtonBox::Ok));
    }
};

void ConfigureCtyDialogTest::initTestCase() {
    m_app = new GLogApplication();
    m_app->hide();
    (void)GLogApplication::getCtyDBInstance()->loadLocalDB();
}

void ConfigureCtyDialogTest::cleanupTestCase() {
    (void)GLogApplication::getCtyDBInstance()->loadLocalDB();
    delete m_app;
    m_app = nullptr;
}

void ConfigureCtyDialogTest::test_success_msg_non_empty() {
    QVERIFY(!ConfigureCtyDialog::successMsg().isEmpty());
}

void ConfigureCtyDialogTest::test_set_db_hint_updates_line_edit() {
    ConfigureCtyDialog dlg;
    dlg.show();
    drainUi();

    const QString hint = QStringLiteral("file:///test/db/hint");
    dlg.setDBhint(hint);

    auto *le = dlg.findChild<QLineEdit *>(QStringLiteral("lineEdit"));
    QVERIFY(le);
    QCOMPARE(le->text(), hint);
}

void ConfigureCtyDialogTest::test_disable_enable_ok_button() {
    ConfigureCtyDialog dlg;
    dlg.show();
    drainUi();

    auto *ok = okButton(dlg);
    QVERIFY(ok);
    QVERIFY(ok->isEnabled());

    dlg.disableCtyConfigure();
    QVERIFY(!ok->isEnabled());

    dlg.enableCtyConfigure();
    QVERIFY(ok->isEnabled());
}

void ConfigureCtyDialogTest::test_ok_loads_local_minimal_cty_without_browse() {
    QTemporaryFile tf(QStringLiteral("%1/cty_test_XXXXXX.dat").arg(QDir::tempPath()));
    QVERIFY(tf.open());
    const QByteArray utf8 = minimalCtyDat().toUtf8();
    QCOMPARE(tf.write(utf8), qint64(utf8.size()));
    QVERIFY(tf.flush());
    const QString localPath = tf.fileName();
    tf.close();

    ConfigureCtyDialog dlg;
    dlg.show();
    drainUi();

    dlg.setDBhint(QStringLiteral("urn:cty-test-prior-hint"));
    auto *le = dlg.findChild<QLineEdit *>(QStringLiteral("lineEdit"));
    QVERIFY(le);
    le->setText(QUrl::fromLocalFile(localPath).toString());

    QSignalSpy endSpy(&dlg, &ConfigureCtyDialog::endLoadDB);
    QSignalSpy finishedSpy(&dlg, &ConfigureCtyDialog::loadFinishedInformation);

    auto *ok = okButton(dlg);
    QVERIFY(ok);
    QTest::mouseClick(ok, Qt::LeftButton);
    drainUi();

    QTRY_VERIFY(endSpy.count() >= 1);
    QTRY_VERIFY(finishedSpy.count() >= 1);

    QCOMPARE(dlg.result(), int(QDialog::Accepted));
    auto *ctydb = GLogApplication::getCtyDBInstance();
    QVERIFY(ctydb->ready());
    QVERIFY(ctydb->getDBHint().contains(localPath));

    (void)ctydb->loadLocalDB();
}

QTEST_MAIN(ConfigureCtyDialogTest)
#include "tst_configurectydialog.moc"
