HEADERS += glwidget.h
SOURCES += glwidget.cpp main.cpp

QT += opengl widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/samplebuffers
INSTALLS += target

contains(QT_CONFIG, opengles.) {
    contains(QT_CONFIG, angle): \
        warning("Qt was built with ANGLE, which provides only OpenGL ES 2.0 on top of DirectX 9.0c")
    error("This example requires Qt to be configured with -opengl desktop")
}
