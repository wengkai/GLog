#include <QtTest>
#include <QCoreApplication>
#include "GLogApplication.h"
#include "test_utils.h"

class TestMainWindow : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase() { qDebug("Initializing Test Suite..."); }

    void testWindowTitle() {
        GLogApplication w;
        w.setWindowTitle("Test Title");
        QCOMPARE(w.windowTitle(), QString("Test Title"));
    }

    void testLogic() {
        GLogApplication w;
        QVERIFY(w.isVisible() == false);
        qDebug() << "Successfully created GLogApplication with UI!";
    }

    void cleanupTestCase() { qDebug("Cleaning up..."); }
};

int main(int argc, char *argv[]) {
    // std::cout << "Current PATH: " << std::getenv("PATH") << std::endl;
    QApplication app(argc, argv);
    TestMainWindow tc;
    return QTest::qExec(&tc, argc, argv);
}
#include "hello_ctest.moc"