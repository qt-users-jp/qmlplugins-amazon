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

#include "s3.h"
#include "hmac_sha1.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDebug>
#include <QtCore/QDateTime>
#include <QtCore/QLocale>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>


QNetworkAccessManager *AbstractApi::networkAccessManager = 0;

class AbstractApi::Private : public QObject
{
    Q_OBJECT
public:
    Private(AbstractApi *parent);

    void exec(QNetworkRequest request, QNetworkAccessManager::Operation operation, const QByteArray &data);

private slots:
    void finished();
    void error(QNetworkReply::NetworkError err);

private:
    QByteArray toString(const QDateTime &dt) const;
    QByteArray toString(QNetworkAccessManager::Operation operation) const;
    QByteArray toString(const QUrl &url) const;

private:
    AbstractApi *q;
};

AbstractApi::Private::Private(AbstractApi *parent)
    : QObject(parent)
    , q(parent)
{
    qDebug() << Q_FUNC_INFO << __LINE__ << parent;
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

    QLocale locale("C");
    return QString("%1 %2%3").arg(locale.toString(dt, "ddd, dd MMM yyyy hh:mm:ss")).arg(sign).arg(timezoneHours * 100 + timezoneMinutes, 4, 10, QLatin1Char('0')).toLatin1();
}

QByteArray AbstractApi::Private::toString(QNetworkAccessManager::Operation operation) const
{
    QByteArray ret;
    switch (operation) {
    case QNetworkAccessManager::HeadOperation:
        ret = "HEAD";
        break;
    case QNetworkAccessManager::GetOperation:
        ret = "GET";
        break;
    case QNetworkAccessManager::PostOperation:
        ret = "POST";
        break;
    case QNetworkAccessManager::PutOperation:
        ret = "PUT";
        break;
    case QNetworkAccessManager::DeleteOperation:
        ret = "DELETE";
        break;
    default:
        break;
    }
    return ret;
}

QByteArray AbstractApi::Private::toString(const QUrl &url) const
{
    QByteArray ret;
    QRegularExpression bucket("^([a-z0-9\\-]+)\\.s3[a-z0-9\\-]*\\.amazonaws\\.com$");
    QRegularExpressionMatch match = bucket.match(url.host());
    if (match.hasMatch()) {
        ret.append("/");
        ret.append(match.captured(1));
    }
    ret.append(url.path());
    QString query = url.query();
    if (query.indexOf('&') < query.indexOf('=')) {
        ret.append(QString("?%1").arg(query.left(query.indexOf("&"))));
    }
    return ret;
}

void AbstractApi::Private::exec(QNetworkRequest request, QNetworkAccessManager::Operation operation, const QByteArray &data)
{
    if (!q->m_account) {
        qWarning() << "account is not set.";
        return;
    }
    q->loading(true);
    QNetworkReply *reply = 0;

    QByteArray httpVerb = toString(operation);
    qDebug() << Q_FUNC_INFO << __LINE__ << httpVerb;
    QByteArray contentMd5;
    if (!data.isEmpty())
        contentMd5 = QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
    qDebug() << Q_FUNC_INFO << __LINE__ << contentMd5;
    QByteArray contentType = request.header(QNetworkRequest::ContentTypeHeader).toByteArray();
    qDebug() << Q_FUNC_INFO << __LINE__ << contentType;
    QByteArray date = toString(QDateTime::currentDateTime());
    qDebug() << Q_FUNC_INFO << __LINE__ << date;
    QMap<QString, QString> xamzHeaders;
//    foreach (const QString &key, qSort(xamzHeaders.keys())) {

//    }
    QByteArray canonicalizedAmzHeaders;
    QByteArray canonicalizedResource = toString(request.url());
    qDebug() << Q_FUNC_INFO << __LINE__ << canonicalizedResource;

    QByteArray stringToSign = httpVerb + "\n"
            + contentMd5 + "\n"
            + contentType + "\n"
            + date + "\n"
            + canonicalizedAmzHeaders
            + canonicalizedResource;
    qDebug() << q;
    qDebug() << Q_FUNC_INFO << __LINE__ << q->m_account->awsSecretAccessKey() << stringToSign;
    QByteArray signature = hmac_sha1(q->m_account->awsSecretAccessKey(), stringToSign);
    signature = signature.toBase64();
    QByteArray authorization("AWS ");
    authorization.append(q->m_account->awsAccessKeyId());
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

    qDebug() << Q_FUNC_INFO << __LINE__;

    connect(reply, &QNetworkReply::finished, this, &AbstractApi::Private::finished);
//    connect(reply, &QNetworkReply::error, this, &AbstractApi::Private::error);
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error(QNetworkReply::NetworkError)));
}

void AbstractApi::Private::finished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    q->done(reply->readAll());
    reply->deleteLater();
    q->loading(false);
}

void AbstractApi::Private::error(QNetworkReply::NetworkError err)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    qDebug() << Q_FUNC_INFO << __LINE__ << err << reply->errorString();
}

AbstractApi::AbstractApi(QObject *parent)
    : QObject(parent)
    , d(new Private(this))
    , m_loading(false)
    , m_account(0)
{
    qDebug() << Q_FUNC_INFO << __LINE__;
}

AbstractApi::~AbstractApi()
{
    qDebug() << Q_FUNC_INFO << __LINE__;
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

#include "abstractapi.moc"
