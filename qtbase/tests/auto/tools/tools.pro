TEMPLATE=subdirs
SUBDIRS=\
   qmake \
   uic \
   moc \
   rcc \

qtHaveModule(dbus): SUBDIRS += qdbuscpp2xml qdbusxml2cpp
!qtHaveModule(widgets): SUBDIRS -= uic
