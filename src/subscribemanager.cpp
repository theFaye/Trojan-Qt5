#include "subscribemanager.h"
#include "trojanvalidator.h"
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QEventLoop>
#include <QDebug>

SubscribeManager::SubscribeManager(ConfigHelper *ch, QObject *parent) : QObject(parent), helper(ch)
{}

QString SubscribeManager::checkUpdate(QString url, bool useProxy)
{
    QNetworkAccessManager* manager = new QNetworkAccessManager();
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", QString("Trojan-Qt5/%1").arg(APP_VERSION).toUtf8());
    if (useProxy) {
        QNetworkProxy proxy;
        proxy.setType(QNetworkProxy::Socks5Proxy);
        proxy.setHostName("127.0.0.1");
        proxy.setPort(helper->getSocks5Port());
        manager->setProxy(proxy);
    }
    QNetworkReply* reply = manager->sendCustomRequest(request, "GET", "");
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    return QString::fromUtf8(reply->readAll());
}

void SubscribeManager::updateAllSubscribes(bool useProxy)
{
    QList<TQSubscribe> subscribes = helper->readSubscribes();
    for (int i = 0; i < subscribes.size(); i++) {
        subscribes[i].lastUpdateTime = QDateTime::currentDateTime().toTime_t() - QDateTime::fromString("1970-01-01T00:00:00").toTime_t();
        QString data = checkUpdate(subscribes[i].url, useProxy);
        QByteArray decodeArray = QByteArray::fromBase64(data.toUtf8().data());
        QString decodeRes = QUrl::fromPercentEncoding(decodeArray); // remove percentage in uri
        decodeRes = decodeRes.replace("\\r", "\r"); // change \\r to \r
        decodeRes = decodeRes.replace("\\n", "\n"); // change \\n to \n
        decodeRes = decodeRes.replace("\r\n", "\n"); // change \r\n to \n
        QStringList list = decodeRes.split("\n");
        for (int i = 0; i< list.length(); i++)
            if (TrojanValidator::validate(list[i]))
                emit addUri(list[i]);
    }
    helper->saveSubscribes(subscribes);
}
