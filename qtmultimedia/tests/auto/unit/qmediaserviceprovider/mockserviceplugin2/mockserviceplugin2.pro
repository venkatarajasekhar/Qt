TARGET = mockserviceplugin2
QT += multimedia-private

PLUGIN_TYPE=mediaservice
PLUGIN_CLASS_NAME = MockServicePlugin2
load(qt_plugin)

DESTDIR = ../$${PLUGIN_TYPE}
win32 {
    CONFIG(debug, debug|release) {
        DESTDIR = ../debug/$${PLUGIN_TYPE}
    } else {
        DESTDIR = ../release/$${PLUGIN_TYPE}
    }
}

HEADERS += ../mockservice.h
SOURCES += mockserviceplugin2.cpp
OTHER_FILES += mockserviceplugin2.json

target.path = $$[QT_INSTALL_TESTS]/tst_qmediaserviceprovider/$${PLUGIN_TYPE}

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
