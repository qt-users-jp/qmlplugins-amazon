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

#include "bucket.h"

#include <QtCore/QDateTime>

Bucket::Bucket(QObject *parent)
    : AbstractApi(parent)
    , m_maxKeys(0)
    , m_isTruncated(false)
{
    connect(this, SIGNAL(accountChanged(S3*)), this, SLOT(get()));
    connect(this, &Bucket::nameChanged, this, &Bucket::get);
}

void Bucket::get()
{
    if (account() && !m_name.isEmpty()) {
        qDebug() << Q_FUNC_INFO << __LINE__ << m_name;
        exec(QNetworkRequest(QUrl(QString("http://%1.s3.amazonaws.com/").arg(m_name))), QNetworkAccessManager::GetOperation);
        qDebug() << Q_FUNC_INFO << __LINE__ << m_name;
    }
}

void Bucket::done(const QByteArray &data)
{
    qDebug() << Q_FUNC_INFO << __LINE__ << data;
    QXmlStreamReader xml(data);

    QVariantMap map;
    QVariantList list;
    while (!xml.atEnd()) {
        QXmlStreamReader::TokenType type = xml.readNext();
        qDebug() << Q_FUNC_INFO << __LINE__ << type << xml.tokenString();
        qDebug() << Q_FUNC_INFO << __LINE__ << xml.name();
          switch (type) {
          case QXmlStreamReader::StartElement:
              qDebug() << xml.name() << xml.text().toString();
              if (xml.name() == QLatin1String("ID")) {
                  xml.readNext();
                  map.insert("id", xml.text().toString());
              } else if (xml.name() == QLatin1String("DisplayName")) {
                  xml.readNext();
                  map.insert("displayName", xml.text().toString());
              } else if (xml.name() == QLatin1String("Name")) {
                  xml.readNext();
                  map.insert("name", xml.text().toString());
              } else if (xml.name() == QLatin1String("CreationDate")) {
                  xml.readNext();
                  map.insert("creationDate", QDateTime::fromString(xml.text().toString(), QLatin1String("yyyy-MM-ddThh:mm:ss.zzzZ")));
              }
              break;
          case QXmlStreamReader::EndElement:
              if (xml.name() == QLatin1String("Owner")) {
//                  owner(map);
                  map.clear();
              } else if (xml.name() == QLatin1String("Bucket")) {
                  list.append(map);
                  map.clear();
              } else if (xml.name() == QLatin1String("Buckets")) {
                  qDebug() << Q_FUNC_INFO << __LINE__ << list;
//                  buckets(list);
                  list.clear();
              }
              break;
          default:
              break;
          }

//          qDebug() << Q_FUNC_INFO << __LINE__ << xml.tokenString();
//          qDebug() << Q_FUNC_INFO << __LINE__ << xml.name() << xml.text();
    }
    if (xml.hasError()) {
    }
}
