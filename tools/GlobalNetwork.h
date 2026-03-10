#ifndef GLOBALNETWORK_H
#define GLOBALNETWORK_H

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>

namespace GLogNetwork {
    QNetworkAccessManager * instance();
    void init(QObject * parent);
    QNetworkReply * get(const QString & url);
    template<typename Done>
    void get(const QString & url, Done done) {
        auto rep = get(url);
        QObject::connect(rep, &QNetworkReply::finished, [=](){
            done(rep);
            rep->deleteLater();
        });
    }
}

#endif