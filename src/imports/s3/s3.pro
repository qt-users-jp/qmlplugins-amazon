IMPORT_VERSION = 0.1
TARGET = amazons3
QT = qml amazons3
LIBS += -L$$QT.amazons3.libs

SOURCES += main.cpp

load(qml_plugin)

OTHER_FILES = plugins.qmltypes qmldir
