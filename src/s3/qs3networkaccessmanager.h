#ifndef S3NETWORKACCESSMANAGER_H
#define S3NETWORKACCESSMANAGER_H

#include "s3_global.h"

#include <QtNetwork/QNetworkAccessManager>

class S3_EXPORT QS3NetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT
public:
    static QS3NetworkAccessManager &instance();

private:
    explicit QS3NetworkAccessManager(QObject *parent = 0);
};

#endif // S3NETWORKACCESSMANAGER_H
