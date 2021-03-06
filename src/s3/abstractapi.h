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

#ifndef ABSTRACTAPI_H
#define ABSTRACTAPI_H

#include "s3_global.h"

#include <QtCore/QDebug>
#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtCore/QXmlStreamReader>

class QAccount;

class S3_EXPORT AbstractApi : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAccount *account READ account WRITE setAccount NOTIFY accountChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
public:
    explicit AbstractApi(QObject *parent = 0);

    static void setNetworkAccessManager(QNetworkAccessManager *networkAccessManager);

    QAccount *account() const;
    bool loading() const;
    int progress() const;

public slots:
    void setAccount(QAccount *account);

signals:
    void accountChanged(QAccount *account);
    void loadingChanged(bool loading);
    void progressChanged(int progress);

protected:
    void exec(QNetworkRequest request, QNetworkAccessManager::Operation method, const QByteArray &data = QByteArray());
    virtual void done(QIODevice *io) = 0;

private:
    void setLoading(bool loading);
    void setProgress(int progress);

    class Private;
    Private *d;

    static QNetworkAccessManager *networkAccessManager;
};

#endif // ABSTRACTAPI_H
