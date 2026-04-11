#include <QtTest>
#include <QCoreApplication>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslSocket>
#include <iostream>
#include "GLogApplication.h"
#include "GlobalNetwork.h"
#include "test_utils.h"

class CtyDBTest : public QObject {
    Q_OBJECT

    bool networkOk = false;
    CtyDB *ctydb = nullptr;

  private slots:
    void initTestCase() {
        qDebug("Initializing Test Suite...");
        {
            QNetworkAccessManager manager;
            auto req = QNetworkRequest(QUrl("https://www.country-files.com/"));
            GLogNetwork::setGeneralHeader(req);
            QNetworkReply *reply = manager.get(req);
            QEventLoop loop;
            QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

            QTimer::singleShot(5000, &loop, &QEventLoop::quit);
            loop.exec();

            if (reply->isFinished() && reply->error() == QNetworkReply::NoError) {
                networkOk = true;
            } else {
                if (reply->isRunning()) {
                    reply->abort();
                }
            }
            reply->deleteLater();
        }
        QCoreApplication::processEvents();
    }

    void testCtyDBInit() {
        {
            QEventLoop loop;
            bool done = false;
            GLogApplication w;
            QObject::connect(&w, &GLogApplication::initCtyDBDone, [&]() {
                loop.quit();
                done = true;
            });
            if (!done) {
                QTimer::singleShot(10000, &loop, [&]() {
                    loop.quit();
                    QFAIL("Database initialization timed out!");
                });
                loop.exec();
            }
            ctydb = w.getCtyDBInstance();
            if (networkOk && !ctydb->getDBHint().startsWith("https:")) {
                QVERIFY2(false, "Network feature incomplete.");
            }
        }
        QCoreApplication::processEvents();
    }

    void verifyCtyDB() {
        auto ret = ctydb->lookUpCallSign(QString::fromLocal8Bit("BG4CPY"));
        QVERIFY(ret.first->vaild);
        QVERIFY(ret.first->name == "China");
    }

    void cleanupTestCase() {
        qDebug("Cleaning up...");
        QCoreApplication::processEvents();
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    CtyDBTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "ctydb_test.moc"