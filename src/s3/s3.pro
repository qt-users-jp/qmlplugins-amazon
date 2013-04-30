TARGET = QtAmazonS3
MODULE = amazons3
QT = core network

load(qt_module)

PUBLIC_HEADERS = qaccount.h qservice.h qbucket.h
HEADERS = $$PUBLIC_HEADERS \
    qabstracts3model.h \
    qs3networkaccessmanager.h
SOURCES = qaccount.cpp qservice.cpp qbucket.cpp \
    qabstracts3model.cpp \
    qs3networkaccessmanager.cpp

DEFINES += S3_LIBRARY

HEADERS += \
    abstractapi.h \
    s3_global.h

SOURCES += \
    abstractapi.cpp
