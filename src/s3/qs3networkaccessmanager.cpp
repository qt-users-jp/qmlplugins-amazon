#include "qs3networkaccessmanager.h"

QS3NetworkAccessManager &QS3NetworkAccessManager::instance()
{
    static QS3NetworkAccessManager ret;
    return ret;
}

QS3NetworkAccessManager::QS3NetworkAccessManager(QObject *parent)
    : QNetworkAccessManager(parent)
{
}
