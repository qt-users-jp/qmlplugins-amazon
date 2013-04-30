#ifndef QABSTRACTS3MODEL_H
#define QABSTRACTS3MODEL_H

#include "s3_global.h"

#include <QtCore/QAbstractListModel>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>

class QAccount;

class S3_EXPORT QAbstractS3Model : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QAccount *account READ account WRITE setAccount NOTIFY accountChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)

    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    explicit QAbstractS3Model(QObject *parent = 0);

    virtual int rowCount(const QModelIndex &parent) const;
    virtual QVariant data(const QModelIndex &index, int role) const;
    
    QAccount *account() const;
    bool loading() const;
    int progress() const;

    int count() const;
    Q_INVOKABLE QVariantMap get(int i) const;

public slots:
    void setAccount(QAccount *account);
private slots:
    void setLoading(bool loading);
    void setProgress(int progress);

signals:
    void accountChanged(QAccount *account);
    void loadingChanged(bool loading);
    void progressChanged(int progress);
    void countChanged(int count);

protected:
    void start(const QUrl &url, QNetworkAccessManager::Operation method, const QByteArray &data = QByteArray());
    virtual void finished(QIODevice *io) = 0;
    void append(const QList<QVariantMap> &data);

private:
    class Private;
    Private *d;
};

#endif // QABSTRACTS3MODEL_H
