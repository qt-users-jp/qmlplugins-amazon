/* Copyright (c) 2012 Silk Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Silk nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SILK BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "abstractapi.h"

#include "qaccount.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QMessageAuthenticationCode>
#include <QtCore/QDebug>
#include <QtCore/QDateTime>
#include <QtCore/QLocale>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>


QNetworkAccessManager *AbstractApi::networkAccessManager = 0;

class AbstractApi::Private
{
public:
    Private(AbstractApi *parent);

    void exec(QNetworkRequest request, QNetworkAccessManager::Operation operation, const QByteArray &data);

private:
    QByteArray toString(const QDateTime &dt) const;
    QByteArray toString(QNetworkAccessManager::Operation operation) const;
    QByteArray toString(const QUrl &url) const;

private:
    AbstractApi *q;

public:
    QAccount *account;
    bool loading;
    int progress;
};

AbstractApi::Private::Private(AbstractApi *parent)
    : q(parent)
    , account(0)
    , loading(false)
    , progress(0)
{
}

QByteArray AbstractApi::Private::toString(const QDateTime &dt) const
{
    QDateTime utc(dt);
    utc.setTimeSpec(Qt::UTC);
    int timezoneSeconds = dt.secsTo(utc);
    QChar sign = (timezoneSeconds >=0 ? QLatin1Char('+') : QLatin1Char('-'));
    if (timezoneSeconds < 0)
        timezoneSeconds = -timezoneSeconds;
    int timezoneMinutes = (timezoneSeconds % 3600) / 60;
    int timezoneHours = (timezoneSeconds / 3600);

    QLocale locale(QStringLiteral("C"));
    return QStringLiteral("%1 %2%3").arg(locale.toString(dt, QStringLiteral("ddd, dd MMM yyyy hh:mm:ss"))).arg(sign).arg(timezoneHours * 100 + timezoneMinutes, 4, 10, QLatin1Char('0')).toLatin1();
}

QByteArray AbstractApi::Private::toString(QNetworkAccessManager::Operation operation) const
{
    static QMap<QNetworkAccessManager::Operation, QByteArray> map;
    if (map.isEmpty()) {
        map.insert(QNetworkAccessManager::HeadOperation, "HEAD");
        map.insert(QNetworkAccessManager::GetOperation, "GET");
        map.insert(QNetworkAccessManager::PostOperation, "POST");
        map.insert(QNetworkAccessManager::PutOperation, "PUT");
        map.insert(QNetworkAccessManager::DeleteOperation, "DELETE");
    }
    return map.value(operation);
}

QByteArray AbstractApi::Private::toString(const QUrl &url) const
{
    QByteArray ret;
    QRegularExpression bucket(QStringLiteral("^([a-z0-9\\-]+)\\.s3[a-z0-9\\-]*\\.amazonaws\\.com$"));
    QRegularExpressionMatch match = bucket.match(url.host());
    if (match.hasMatch()) {
        ret.append("/");
        ret.append(match.captured(1).toUtf8());
    }
    ret.append(url.path().toUtf8());
    QString query = url.query();
    if (query.indexOf('&') < query.indexOf('=')) {
        ret.append(QStringLiteral("?%1").arg(query.left(query.indexOf(QStringLiteral("&")))).toUtf8());
    }
    return ret;
}

void AbstractApi::Private::exec(QNetworkRequest request, QNetworkAccessManager::Operation operation, const QByteArray &data)
{
    if (!account) {
        qWarning() << "account is not set.";
        return;
    }
    q->setLoading(true);
    QNetworkReply *reply = 0;

    QByteArray httpVerb = toString(operation);
    QByteArray contentMd5;
    if (!data.isEmpty())
        contentMd5 = QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
//    qDebug() << Q_FUNC_INFO << __LINE__ << contentMd5;
    QByteArray contentType = request.header(QNetworkRequest::ContentTypeHeader).toByteArray();
//    qDebug() << Q_FUNC_INFO << __LINE__ << contentType;
    QByteArray date = toString(QDateTime::currentDateTime());
//    qDebug() << Q_FUNC_INFO << __LINE__ << date;
//    QMap<QString, QString> xamzHeaders;
//    foreach (const QString &key, qSort(xamzHeaders.keys())) {

//    }
    QByteArray canonicalizedAmzHeaders;
    QByteArray canonicalizedResource = toString(request.url());
//    qDebug() << Q_FUNC_INFO << __LINE__ << canonicalizedResource;

    QByteArray stringToSign = httpVerb + "\n"
            + contentMd5 + "\n"
            + contentType + "\n"
            + date + "\n"
            + canonicalizedAmzHeaders
            + canonicalizedResource;
//    qDebug() << q;
//    qDebug() << Q_FUNC_INFO << __LINE__ << account->awsSecretAccessKey() << stringToSign;
    QByteArray signature = QMessageAuthenticationCode::hash(stringToSign, account->awsSecretAccessKey(), QCryptographicHash::Sha1);
    signature = signature.toBase64();
    QByteArray authorization("AWS ");
    authorization.append(account->awsAccessKeyId());
    authorization.append(":");
    authorization.append(signature);

    request.setRawHeader("Date", date);
    request.setRawHeader("Authorization", authorization);

    if (!networkAccessManager)
        setNetworkAccessManager(new QNetworkAccessManager);

    switch (operation) {
    case QNetworkAccessManager::HeadOperation:
        reply = networkAccessManager->head(request);
        break;
    case QNetworkAccessManager::GetOperation:
        reply = networkAccessManager->get(request);
        break;
    case QNetworkAccessManager::PostOperation:
        reply = networkAccessManager->post(request, "");
        break;
    case QNetworkAccessManager::PutOperation:
        reply = networkAccessManager->put(request, "");
        break;
    case QNetworkAccessManager::DeleteOperation:
        reply = networkAccessManager->deleteResource(request);
        break;
    default:
        break;
    }

    q->setProgress(0);

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        q->done(reply);
        reply->deleteLater();
        q->setLoading(false);
    });
    connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), [this, reply](QNetworkReply::NetworkError error) {
        qDebug() << Q_FUNC_INFO << __LINE__ << error << reply->errorString();
    });
    connect(reply, &QNetworkReply::downloadProgress, [this](qint64 bytesReceived, qint64 bytesTotal) {
        if (bytesTotal > 0)
            q->setProgress(bytesReceived * 100 / bytesTotal);
    });
}

AbstractApi::AbstractApi(QObject *parent)
    : QObject(parent)
    , d(new Private(this))
{
    connect(this, &AbstractApi::destroyed, [d]() { delete d; });
}

QAccount *AbstractApi::account() const
{
    return d->account;
}

void AbstractApi::setAccount(QAccount *account)
{
    if (d->account == account) return;
    d->account = account;
    emit accountChanged(account);
}

bool AbstractApi::loading() const
{
    return d->loading;
}

void AbstractApi::setLoading(bool loading)
{
    if (d->loading == loading) return;
    d->loading = loading;
    emit loadingChanged(loading);
}

int AbstractApi::progress() const
{
    return d->progress;
}

void AbstractApi::setProgress(int progress)
{
    if (d->progress == progress) return;
    d->progress = progress;
    emit progressChanged(progress);
}

void AbstractApi::setNetworkAccessManager(QNetworkAccessManager *nam)
{
    if (networkAccessManager && !networkAccessManager->parent())
        networkAccessManager->deleteLater();
    networkAccessManager = nam;
}

void AbstractApi::exec(QNetworkRequest request, QNetworkAccessManager::Operation operation, const QByteArray &data)
{
    d->exec(request, operation, data);
}
