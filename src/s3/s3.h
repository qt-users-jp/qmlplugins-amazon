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

#ifndef S3_H
#define S3_H

#include <QtCore/QObject>

class S3 : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QByteArray awsAccessKeyId READ awsAccessKeyId WRITE awsAccessKeyId NOTIFY awsAccessKeyIdChanged)
    Q_PROPERTY(QByteArray awsSecretAccessKey READ awsSecretAccessKey WRITE awsSecretAccessKey NOTIFY awsSecretAccessKeyChanged)

public:
    explicit S3(QObject *parent = 0);

signals:
    void awsAccessKeyIdChanged(const QByteArray &awsAccessKeyId);
    void awsSecretAccessKeyChanged(const QByteArray &awsSecretAccessKey);

private:
    class Private;
    Private *d;

#define ADD_PROPERTY(type, name, type2) \
public: \
    type name() const { return m_##name; } \
    void name(type name) { \
        if (m_##name == name) return; \
        m_##name = name; \
        emit name##Changed(name); \
    } \
private: \
    type2 m_##name;

    ADD_PROPERTY(const QByteArray &, awsAccessKeyId, QByteArray)
    ADD_PROPERTY(const QByteArray &, awsSecretAccessKey, QByteArray)
#undef ADD_PROPERTY

};

#endif // S3_H
