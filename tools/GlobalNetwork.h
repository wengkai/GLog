#ifndef GLOBALNETWORK_H
#define GLOBALNETWORK_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include "app_export.h"

namespace GLogNetwork {
GLOGKIT_API QNetworkAccessManager *instance();
GLOGKIT_API void init(QObject *parent);
GLOGKIT_API QNetworkReply *get(const QString &url);
GLOGKIT_API void setGeneralHeader(QNetworkRequest &req);
template <typename Done> void get(const QString &url, Done done) {
    auto rep = get(url);
    QObject::connect(rep, &QNetworkReply::finished, [=]() {
        done(rep);
        rep->deleteLater();
    });
}
template <typename Watcher> void watchGetProcess(QNetworkReply *reply, Watcher watcher) {
    QObject::connect(
        reply, &QNetworkReply::downloadProgress, [=](qint64 bytesReceived, qint64 bytesTotal) {
            QString progress;
            if (bytesTotal > 0) {
                progress = QString("%1%").arg(double(bytesReceived) * 100 / bytesTotal);
            } else {
                progress = QString("%1 bytes").arg(bytesReceived);
            }
            watcher(progress);
        });
}
} // namespace GLogNetwork

#endif