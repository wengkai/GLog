#include <QtTest>
#include <QCoreApplication>
#include "GLogApplication.h"
#include "test_utils.h"

class TestMainWindow : public QObject {
    Q_OBJECT

    GLogApplication *m_app = nullptr;

  private slots:
    void initTestCase() {
        qDebug("Initializing Test Suite...");
        m_app = new GLogApplication();
    }

    void testWindowTitle() {
        m_app->setWindowTitle("Test Title");
        QCOMPARE(m_app->windowTitle(), QString("Test Title"));
    }

    void testLogic() {
        QVERIFY(m_app->isVisible() == false);
        qDebug() << "Successfully created GLogApplication with UI!";
    }

    void cleanupTestCase() {
        qDebug("Cleaning up...");
        delete m_app;
        m_app = nullptr;
    }
};

QTEST_MAIN(TestMainWindow)
#include "tst_hello_app.moc"