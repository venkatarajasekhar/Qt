CONFIG += testcase
TARGET = tst_qdbusreply
QT = core dbus testlib
SOURCES += tst_qdbusreply.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

macx:CONFIG += insignificant_test # QTBUG-37469
