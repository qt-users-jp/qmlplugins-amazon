TEMPLATE = lib
CONFIG += plugin

QT = core network qml

TARGET = s3-qml
TARGETPATH = me/qtquick/Amazon/S3

HEADERS += \
    plugin.h \
    s3.h \
    service.h \
    abstractapi.h \
    hmac_sha1.h \
    bucket.h

SOURCES += \
    s3.cpp \
    service.cpp \
    abstractapi.cpp \
    hmac_sha1.cpp \
    bucket.cpp

target.path = $$[QT_INSTALL_QML]/$$TARGETPATH

qmldir.files = qmldir
qmldir.path = $$[QT_INSTALL_QML]/$$TARGETPATH

INSTALLS = target qmldir

OTHER_FILES += qmldir
