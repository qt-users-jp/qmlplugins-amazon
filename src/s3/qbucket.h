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

#ifndef QBUCKET_H
#define QBUCKET_H

#include "qabstracts3model.h"

#include <QtCore/QVariantMap>
#include <QtCore/QVariantList>

class S3_EXPORT QBucket : public QAbstractS3Model
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString delimiter READ delimiter WRITE setDelimiter NOTIFY delimiterChanged)
    Q_PROPERTY(QString marker READ marker WRITE setMarker NOTIFY markerChanged)
    Q_PROPERTY(int maxKeys READ maxKeys WRITE setMaxKeys NOTIFY maxKeysChanged)
    Q_PROPERTY(QString prefix READ prefix WRITE setPrefix NOTIFY prefixChanged)
    Q_PROPERTY(bool isTruncated READ isTruncated NOTIFY truncatedChanged)
public:
    explicit QBucket(QObject *parent = 0);

    virtual QHash<int, QByteArray> roleNames() const;

    const QString &name() const;
    const QString &delimiter() const;
    const QString &marker() const;
    int maxKeys() const;
    const QString &prefix() const;
    bool isTruncated() const;

public slots:
    void setName(const QString &name);
    void setDelimiter(const QString &delmiter);
    void setMarker(const QString &marker);
    void setMaxKeys(int maxKeys);
    void setPrefix(const QString &prefix);

private slots:
    void setTruncated(bool trunctated);

public slots:
    void load();

signals:
    void nameChanged(const QString &name);
    void delimiterChanged(const QString &delimiter);
    void markerChanged(const QString &marker);
    void maxKeysChanged(int maxKeys);
    void prefixChanged(const QString &prefix);
    void truncatedChanged(bool isTruncated);

protected:
    void finished(QIODevice *io);

private:
    class Private;
    Private *d;
};

#endif // QBUCKET_H
