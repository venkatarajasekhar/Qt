import QtQuick 1.0

QtObject {
    property bool exceptionThrown: false

    Component.onCompleted: {
        var x = new XMLHttpRequest;

        try {
            x.open("GET", "http://www.qt-project.org", false);
        } catch (e) {
            if (e.code == DOMException.NOT_SUPPORTED_ERR)
                exceptionThrown = true;
        }
    }
}

