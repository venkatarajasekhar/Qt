TEMPLATE = app
TARGET = qquickviewcomparison

QT += quick widgets quickwidgets

SOURCES += main.cpp \
           mainwindow.cpp \
           logo.cpp \
           fbitem.cpp

HEADERS += mainwindow.h \
           logo.h \
           fbitem.h

RESOURCES += qquickviewcomparison.qrc

OTHER_FILES += test.qml
