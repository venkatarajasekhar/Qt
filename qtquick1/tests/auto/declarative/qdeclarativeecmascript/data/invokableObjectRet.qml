import Qt.test 1.0
import QtQuick 1.0

MyQmlObject {
    id: root
    property bool test: false
    Component.onCompleted: {
        test = (root.returnme() == root)
    }
}

