CONFIG += testcase
TARGET = tst_qdbusmetatype
QT = core dbus testlib
SOURCES += tst_qdbusmetatype.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

macx:CONFIG += insignificant_test # QTBUG-37469
