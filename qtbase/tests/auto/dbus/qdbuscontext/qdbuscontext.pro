CONFIG += testcase
TARGET = tst_qdbuscontext
QT = core dbus testlib
SOURCES += tst_qdbuscontext.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

macx:CONFIG += insignificant_test # QTBUG-37469
