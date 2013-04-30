#include "qabstracts3model.h"

#include "qaccount.h"
#include "qs3networkaccessmanager.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QMessageAuthenticationCode>
#include <QtCore/QDebug>
#include <QtCore/QDateTime>
#include <QtCore/QLocale>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>


class QAbstractS3Model::Private
{
public:
    Private(QAbstractS3Model *parent);

    void start(const QUrl &url, QNetworkAccessManager::Operation operation, const QByteArray &data = QByteArray());

private:
    QByteArray toString(const QDateTime &dt) const;
    QByteArray toString(QNetworkAccessManager::Operation operation) const;
    QByteArray toString(const QUrl &url) const;

private:
    QAbstractS3Model *q;

public:
    QAccount *account;
    bool loading;
    int progress;

    QList<QVariantMap> data;
};

QAbstractS3Model::Private::Private(QAbstractS3Model *parent)
    : q(parent)
    , account(0)
    , loading(false)
    , progress(0)
{
}

QByteArray QAbstractS3Model::Private::toString(const QDateTime &dt) const
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

QByteArray QAbstractS3Model::Private::toString(QNetworkAccessManager::Operation operation) const
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

QByteArray QAbstractS3Model::Private::toString(const QUrl &url) const
{
//    qDebug() << Q_FUNC_INFO << __LINE__ << url;
    QByteArray ret;
    QRegularExpression bucket(QStringLiteral("^([a-z0-9\\-]+)\\.s3[a-z0-9\\-]*\\.amazonaws\\.com$"));
    QRegularExpressionMatch match = bucket.match(url.host());
    if (match.hasMatch()) {
        ret.append("/");
        ret.append(match.captured(1).toUtf8());
    }
    ret.append(url.path().toUtf8());
//    QString query = url.query();
//    if (query.indexOf('&') < query.indexOf('=')) {
//        ret.append(QStringLiteral("?%1").arg(query.left(query.indexOf(QStringLiteral("&")))).toUtf8());
//    }
//    qDebug() << Q_FUNC_INFO << __LINE__ << ret;
    return ret;
}

void QAbstractS3Model::Private::start(const QUrl &url, QNetworkAccessManager::Operation operation, const QByteArray &data)
{
    if (!account) return;
    if (account->awsAccessKeyId().isEmpty()) return;
    if (account->awsSecretAccessKey().isEmpty()) return;

    q->setLoading(true);
    QNetworkReply *reply = 0;

    QByteArray httpVerb = toString(operation);
    QByteArray contentMd5;
    if (!data.isEmpty())
        contentMd5 = QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
//    qDebug() << Q_FUNC_INFO << __LINE__ << contentMd5;
    QNetworkRequest request(url);
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

    switch (operation) {
    case QNetworkAccessManager::HeadOperation:
        reply = QS3NetworkAccessManager::instance().head(request);
        break;
    case QNetworkAccessManager::GetOperation:
        reply = QS3NetworkAccessManager::instance().get(request);
        break;
    case QNetworkAccessManager::PostOperation:
        reply = QS3NetworkAccessManager::instance().post(request, data);
        break;
    case QNetworkAccessManager::PutOperation:
        reply = QS3NetworkAccessManager::instance().put(request, data);
        break;
    case QNetworkAccessManager::DeleteOperation:
        reply = QS3NetworkAccessManager::instance().deleteResource(request);
        break;
    default:
        break;
    }

    q->setProgress(0);

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        int httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        switch (httpStatusCode) {
        case 200:
            q->finished(reply);
            q->setLoading(false);
            break;
        case 307:
            start(reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl(), reply->operation());
            break;
        }
        reply->deleteLater();
    });
    connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), [this, reply](QNetworkReply::NetworkError error) {
        qDebug() << Q_FUNC_INFO << __LINE__ << error << reply->errorString();
        qDebug() << Q_FUNC_INFO << __LINE__ << reply->readAll();
    });
    connect(reply, &QNetworkReply::downloadProgress, [this](qint64 bytesReceived, qint64 bytesTotal) {
        if (bytesTotal > 0)
            q->setProgress(bytesReceived * 100 / bytesTotal);
    });
}

QAbstractS3Model::QAbstractS3Model(QObject *parent)
    : QAbstractListModel(parent)
    , d(new Private(this))
{
    connect(this, &QAbstractS3Model::destroyed, [d]() { delete d; });
}

int QAbstractS3Model::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return d->data.count();
}

QVariant QAbstractS3Model::data(const QModelIndex &index, int role) const
{
    QVariant ret;
    int row = index.row();
    if (0 <= row && row < d->data.count()) {
        ret = d->data.at(row).value(QString::fromUtf8(roleNames().value(role)));
    }
    return ret;
}

QVariantMap QAbstractS3Model::get(int i) const
{
    QVariantMap ret;
    if (0 <= i && i < d->data.count()) {
        ret = d->data.at(i);
    }
    return ret;
}

QAccount *QAbstractS3Model::account() const
{
    return d->account;
}

void QAbstractS3Model::setAccount(QAccount *account)
{
    if (d->account == account) return;
    d->account = account;
    emit accountChanged(account);
}

bool QAbstractS3Model::loading() const
{
    return d->loading;
}

void QAbstractS3Model::setLoading(bool loading)
{
    if (d->loading == loading) return;
    d->loading = loading;
    emit loadingChanged(loading);
}

int QAbstractS3Model::progress() const
{
    return d->progress;
}

void QAbstractS3Model::setProgress(int progress)
{
    if (d->progress == progress) return;
    d->progress = progress;
    emit progressChanged(progress);
}

int QAbstractS3Model::count() const
{
    return d->data.count();
}

void QAbstractS3Model::start(const QUrl &url, QNetworkAccessManager::Operation operation, const QByteArray &data)
{
    d->start(url, operation, data);
}

void QAbstractS3Model::append(const QList<QVariantMap> &data)
{
    if (data.isEmpty()) return;
    beginInsertRows(QModelIndex(), d->data.length(), d->data.length() + data.length() - 1);
    d->data.append(data);
    endInsertRows();
    emit countChanged(d->data.count());
}
