list(APPEND WTF_SOURCES
    efl/MainThreadEfl.cpp
    efl/OwnPtrEfl.cpp
    efl/RefPtrEfl.cpp

    gobject/GOwnPtr.cpp
    gobject/GRefPtr.cpp
)

list(APPEND WTF_LIBRARIES
    pthread
    ${GLIB_LIBRARIES}
    ${GLIB_GIO_LIBRARIES}
    ${GLIB_GOBJECT_LIBRARIES}
    ${ECORE_LIBRARIES}
    ${ECORE_EVAS_LIBRARIES}
    ${ECORE_IMF_LIBRARIES}
    ${EINA_LIBRARIES}
    ${EO_LIBRARIES}
    ${EVAS_LIBRARIES}
)

list(APPEND WTF_INCLUDE_DIRECTORIES
    ${ECORE_EVAS_INCLUDE_DIRS}
    ${ECORE_INCLUDE_DIRS}
    ${ECORE_IMF_INCLUDE_DIRS}
    ${EINA_INCLUDE_DIRS}
    ${EO_INCLUDE_DIRS}
    ${EVAS_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
)
