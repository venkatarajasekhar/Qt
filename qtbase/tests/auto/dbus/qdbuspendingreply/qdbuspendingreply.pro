CONFIG += testcase
TARGET = tst_qdbuspendingreply
QT = core dbus testlib
SOURCES += tst_qdbuspendingreply.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

macx:CONFIG += insignificant_test # QTBUG-37469
