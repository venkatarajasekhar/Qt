TEMPLATE = app

QT += widgets axcontainer

HEADERS  = webaxwidget.h
SOURCES  = main.cpp
FORMS    = mainwindow.ui
wincewm*: FORMS = mainwindow_windowsmobile.ui


# install
target.path = $$[QT_INSTALL_EXAMPLES]/activeqt/webbrowser
INSTALLS += target
