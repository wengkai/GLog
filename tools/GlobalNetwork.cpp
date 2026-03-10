#include "GlobalNetwork.h"

namespace GLogNetwork {
    QNetworkAccessManager *manager = nullptr;
}

QNetworkAccessManager *GLogNetwork::instance()
{
    Q_ASSERT(manager);
    return manager;
}

void GLogNetwork::init(QObject *parent)
{
    if (!manager)
        manager = new QNetworkAccessManager(parent);
}

QNetworkReply *GLogNetwork::get(const QString &url)
{
    Q_ASSERT(manager);
    auto m_url = QUrl(url);
    auto host = m_url.adjusted(QUrl::RemovePath).toString();
    auto req = QNetworkRequest(m_url);
    req.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");
    req.setRawHeader("Referer", host.toUtf8());
    auto rep = manager->get(req);
    return rep;
}
