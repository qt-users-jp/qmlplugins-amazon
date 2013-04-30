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

#include "qbucket.h"

#include <QtCore/QDebug>
#include <QtCore/QDateTime>
#include <QtCore/QTimer>
#include <QtCore/QUrlQuery>
#include <QtCore/QXmlStreamReader>

#include "qaccount.h"

class QBucket::Private
{
public:
    Private(QBucket *parent);

    QString name;
    QString delimiter;
    QString marker;
    int maxKeys;
    QString prefix;
    bool truncated;

    static QHash<int, QByteArray> roleNames;
    QTimer timer;
};

QHash<int, QByteArray> QBucket::Private::roleNames;

QBucket::Private::Private(QBucket *parent)
    : maxKeys(0)
    , truncated(false)
{
    timer.setInterval(0);
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, parent, &QBucket::load);

    connect(parent, &QBucket::accountChanged, &timer, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(parent, &QBucket::nameChanged, &timer, static_cast<void (QTimer::*)()>(&QTimer::start));
}

QBucket::QBucket(QObject *parent)
    : QAbstractS3Model(parent)
    , d(new Private(this))
{
    connect(this, &QBucket::destroyed, [d]() { delete d; });
}

QHash<int, QByteArray> QBucket::roleNames() const
{
    if (d->roleNames.isEmpty()) {
        int role = Qt::UserRole;
        d->roleNames.insert(role++, "key");
        d->roleNames.insert(role++, "lastModified");
        d->roleNames.insert(role++, "eTag");
        d->roleNames.insert(role++, "size");
        d->roleNames.insert(role++, "storageClass");
        d->roleNames.insert(role++, "owner");
    }
    return d->roleNames;
}

const QString &QBucket::name() const
{
    return d->name;
}

void QBucket::setName(const QString &name)
{
    if (d->name == name) return;
    d->name = name;
    emit nameChanged(name);
}

const QString &QBucket::delimiter() const
{
    return d->delimiter;
}

void QBucket::setDelimiter(const QString &delimiter)
{
    if (d->delimiter == delimiter) return;
    d->delimiter = delimiter;
    emit delimiterChanged(delimiter);
}

const QString &QBucket::marker() const
{
    return d->marker;
}

void QBucket::setMarker(const QString &marker)
{
    if (d->marker == marker) return;
    d->marker = marker;
    emit markerChanged(marker);
}

int QBucket::maxKeys() const
{
    return d->maxKeys;
}

void QBucket::setMaxKeys(int maxKeys)
{
    if (d->maxKeys == maxKeys) return;
    d->maxKeys = maxKeys;
    emit maxKeysChanged(maxKeys);
}

const QString &QBucket::prefix() const
{
    return d->prefix;
}

void QBucket::setPrefix(const QString &prefix)
{
    if (d->prefix == prefix) return;
    d->prefix = prefix;
    emit prefixChanged(prefix);
}

bool QBucket::isTruncated() const
{
    return d->truncated;
}

void QBucket::setTruncated(bool truncated)
{
    if (d->truncated == truncated) return;
    d->truncated = truncated;
    emit truncatedChanged(truncated);
}

void QBucket::load()
{
    if (loading()) return;
    if (!account()) return;
    if (d->name.isEmpty()) return;

    QUrl url(QStringLiteral("http://%1.s3.amazonaws.com/").arg(d->name));
    QUrlQuery query;
    if (!d->delimiter.isEmpty())
        query.addQueryItem(QStringLiteral("delimiter"), d->delimiter);
    if (!d->marker.isEmpty())
        query.addQueryItem(QStringLiteral("marker"), d->marker);
    if (d->maxKeys > 0)
        query.addQueryItem(QStringLiteral("max-keys"), QString::number(d->maxKeys));
    if (!d->prefix.isEmpty())
        query.addQueryItem(QStringLiteral("prefix"), d->prefix);
    url.setQuery(query);
    start(url, QNetworkAccessManager::GetOperation);
}

void QBucket::finished(QIODevice *io)
{
    QXmlStreamReader xml(io);

    bool commonPrefix = false;
    QVariantMap content;
    QVariantMap owner;

    QList<QVariantMap> contents;
    QList<QVariantMap> commonPrefixes;

    while (!xml.atEnd()) {
        QXmlStreamReader::TokenType type = xml.readNext();
        switch (type) {
        case QXmlStreamReader::StartElement:
            if (xml.name() == QStringLiteral("ListBucketResult")) {
                contents.clear();
            } else if (xml.name() == QStringLiteral("Name")) {
                xml.readNext();
                setName(xml.text().toString());
            } else if (xml.name() == QStringLiteral("Prefix")) {
                xml.readNext();
                if (commonPrefix) {
                    content.insert(QStringLiteral("key"), xml.text().toString());
                } else {
                    setPrefix(xml.text().toString());
                }
            } else if (xml.name() == QStringLiteral("Delimiter")) {
                xml.readNext();
                setDelimiter(xml.text().toString());
            } else if (xml.name() == QStringLiteral("Marker")) {
                xml.readNext();
                setMarker(xml.text().toString());
            } else if (xml.name() == QStringLiteral("MaxKeys")) {
                xml.readNext();
                setMaxKeys(xml.text().toInt());
            } else if (xml.name() == QStringLiteral("IsTruncated")) {
                xml.readNext();
                setTruncated(xml.text().toString() == QStringLiteral("true"));
            } else if (xml.name() == QStringLiteral("Contents")) {
                content.clear();
            } else if (xml.name() == QStringLiteral("Key")) {
                xml.readNext();
                content.insert(QStringLiteral("key"), xml.text().toString());
            } else if (xml.name() == QStringLiteral("LastModified")) {
                xml.readNext();
                content.insert(QStringLiteral("lastModified"), QDateTime::fromString(xml.text().toString(), QStringLiteral("yyyy-MM-ddThh:mm:ss.zzzZ")));
            } else if (xml.name() == QStringLiteral("ETag")) {
                xml.readNext();
                content.insert(QStringLiteral("eTag"), xml.text().toString());
            } else if (xml.name() == QStringLiteral("Size")) {
                xml.readNext();
                content.insert(QStringLiteral("size"), xml.text().toULongLong());
            } else if (xml.name() == QStringLiteral("StorageClass")) {
                xml.readNext();
                content.insert(QStringLiteral("storageClass"), xml.text().toString());
            } else if (xml.name() == QStringLiteral("Owner")) {
                owner.clear();
            } else if (xml.name() == QStringLiteral("ID")) {
                xml.readNext();
                owner.insert(QStringLiteral("id"), xml.text().toString());
            } else if (xml.name() == QStringLiteral("DisplayName")) {
                xml.readNext();
                owner.insert(QStringLiteral("displayName"), xml.text().toString());
            } else if (xml.name() == QStringLiteral("CommonPrefixes")) {
                commonPrefix = true;
                content.clear();
            } else if (xml.name() == QStringLiteral("Key")) {
                xml.readNext();
                content.insert(QStringLiteral("key"), xml.text().toString());
            } else {
                qDebug() << Q_FUNC_INFO << __LINE__ << xml.name();
                xml.readNext();
                qDebug() << Q_FUNC_INFO << __LINE__ << xml.text().toString();
            }
            break;
        case QXmlStreamReader::EndElement:
            if (xml.name() == QStringLiteral("Owner")) {
                content.insert(QStringLiteral("owner"), owner);
            } else if (xml.name() == QStringLiteral("Contents")) {
                contents.append(content);
            } else if (xml.name() == QStringLiteral("CommonPrefixes")) {
                commonPrefix = false;
                commonPrefixes.append(content);
            } else if (xml.name() == QStringLiteral("ListBucketResult")) {
//                qDebug() << Q_FUNC_INFO << __LINE__ << contents;
                append(commonPrefixes);
                append(contents);
            }
            break;
        default:
            break;
        }
    }
    if (xml.hasError()) {
    }
}
