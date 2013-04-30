TEMPLATE = subdirs
SUBDIRS = s3

!isEmpty(QT.qml.name) {
    src_imports.subdir = imports
    src_imports.depends = s3
    SUBDIRS += src_imports
}

