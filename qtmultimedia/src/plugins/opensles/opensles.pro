TARGET = qtaudio_opensles
QT += multimedia-private

PLUGIN_TYPE = audio
PLUGIN_CLASS_NAME = QOpenSLESPlugin
load(qt_plugin)

LIBS += -lOpenSLES

HEADERS += \
    qopenslesplugin.h \
    qopenslesengine.h \
    qopenslesdeviceinfo.h \
    qopenslesaudioinput.h \
    qopenslesaudiooutput.h

SOURCES += \
    qopenslesplugin.cpp \
    qopenslesengine.cpp \
    qopenslesdeviceinfo.cpp \
    qopenslesaudioinput.cpp \
    qopenslesaudiooutput.cpp

OTHER_FILES += \
    opensles.json
