
INCLUDEPATH += $$PWD

DEFINES += QT_DESIGNER

QT += widgets

# Input
HEADERS += \
    $$PWD/ui4_p.h \
    $$PWD/abstractformbuilder.h \
    $$PWD/formbuilder.h \
    $$PWD/container.h \
    $$PWD/customwidget.h \
    $$PWD/properties_p.h \
    $$PWD/formbuilderextra_p.h \
    $$PWD/resourcebuilder_p.h \
    $$PWD/textbuilder_p.h

SOURCES += \
    $$PWD/abstractformbuilder.cpp \
    $$PWD/formbuilder.cpp \
    $$PWD/ui4.cpp \
    $$PWD/properties.cpp \
    $$PWD/formbuilderextra.cpp \
    $$PWD/resourcebuilder.cpp \
    $$PWD/textbuilder.cpp
