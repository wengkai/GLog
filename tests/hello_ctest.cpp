#include "GLogApplication.h"
#include "test_utils.h"
#include <QtTest>
#include <QCoreApplication>

class TestMainWindow : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        qDebug("Initializing Test Suite...");
    }

    void testWindowTitle() {
        {
            GLogApplication w;
            w.setWindowTitle("Test Title");
            QCOMPARE(w.windowTitle(), QString("Test Title"));
        }
        QCoreApplication::processEvents();
    }

    void testLogic() {
        {
            GLogApplication w;
            QVERIFY(w.isVisible() == false);
            qDebug() << "Successfully created GLogApplication with UI!";
        }
        QCoreApplication::processEvents();
    }

    void cleanupTestCase() {
        qDebug("Cleaning up...");
        QCoreApplication::processEvents();
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    TestMainWindow tc;
    int result = QTest::qExec(&tc, argc, argv);
    std::cout << "Test completed, forcing exit..." << std::endl;
    SystemUtils::quick_exit_program(result); 
}
#include "hello_ctest.moc"