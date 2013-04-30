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

#include "qservice.h"

#include <QtCore/QDebug>
#include <QtCore/QDateTime>
#include <QtCore/QTimer>
#include <QtCore/QXmlStreamReader>

#include "qaccount.h"

class QService::Private
{
public:
    Private(QService *parent);
    QVariantMap owner;
    static QHash<int, QByteArray> roleNames;
    QTimer timer;
};

QHash<int, QByteArray> QService::Private::roleNames;

QService::Private::Private(QService *parent)
{
    timer.setInterval(0);
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, parent, &QService::load);

    connect(parent, &QService::accountChanged, [=](QAccount *account) {
        if (account) {
            if (account->awsAccessKeyId().isEmpty() || account->awsSecretAccessKey().isEmpty()) {
                connect(account, &QAccount::awsAccessKeyIdChanged, &timer, static_cast<void (QTimer::*)()>(&QTimer::start));
                connect(account, &QAccount::awsSecretAccessKeyChanged, &timer, static_cast<void (QTimer::*)()>(&QTimer::start));
            } else {
                timer.start();
            }
        }
    });
}

QService::QService(QObject *parent)
    : QAbstractS3Model(parent)
    , d(new Private(this))
{
    connect(this, &QService::destroyed, [d]() { delete d; });
}

QHash<int, QByteArray> QService::roleNames() const
{
    if (d->roleNames.isEmpty()) {
        int role = Qt::UserRole;
        d->roleNames.insert(role++, "name");
        d->roleNames.insert(role++, "creationDate");
    }
    return d->roleNames;
}

const QVariantMap &QService::owner() const
{
    return d->owner;
}

void QService::setOwner(const QVariantMap &owner)
{
    if (d->owner == owner) return;
    d->owner = owner;
    emit ownerChanged(owner);
}

void QService::load()
{
    if (loading()) return;
    start(QUrl(QStringLiteral("http://s3.amazonaws.com/")), QNetworkAccessManager::GetOperation);
}

void QService::finished(QIODevice *io)
{
    QXmlStreamReader xml(io);

    QVariantMap owner;
    QVariantMap bucket;
    QList<QVariantMap> buckets;
    while (!xml.atEnd()) {
        QXmlStreamReader::TokenType type = xml.readNext();
        switch (type) {
        case QXmlStreamReader::StartElement:
            if (xml.name() == QStringLiteral("Owner")) {
                owner.clear();
            } else if (xml.name() == QStringLiteral("ID")) {
                xml.readNext();
                owner.insert(QStringLiteral("id"), xml.text().toString());
            } else if (xml.name() == QStringLiteral("DisplayName")) {
                xml.readNext();
                owner.insert(QStringLiteral("displayName"), xml.text().toString());
            } else if (xml.name() == QStringLiteral("Buckets")) {
                buckets.clear();
            } else if (xml.name() == QStringLiteral("Bucket")) {
                bucket.clear();
            } else if (xml.name() == QStringLiteral("Name")) {
                xml.readNext();
                bucket.insert(QStringLiteral("name"), xml.text().toString());
            } else if (xml.name() == QStringLiteral("CreationDate")) {
                xml.readNext();
                bucket.insert(QStringLiteral("creationDate"), QDateTime::fromString(xml.text().toString(), QStringLiteral("yyyy-MM-ddThh:mm:ss.zzzZ")));
            }
            break;
        case QXmlStreamReader::EndElement:
            if (xml.name() == QStringLiteral("Owner")) {
                setOwner(owner);
            } else if (xml.name() == QStringLiteral("Bucket")) {
                buckets.append(bucket);
            } else if (xml.name() == QStringLiteral("Buckets")) {
//                setBuckets(buckets);
                append(buckets);
            }
            break;
        default:
            break;
        }
    }
    if (xml.hasError()) {
    }
}
