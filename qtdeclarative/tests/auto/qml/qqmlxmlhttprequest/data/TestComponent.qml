import QtQuick 2.0

Item {
    id: root
    property int a: 4

    Component.onCompleted: {
        root.parent.finished();
        triggerXmlHttpRequest();
    }

    function triggerXmlHttpRequest() {
        var doc = new XMLHttpRequest();
        doc.onreadystatechange = function() {
            if (doc.readyState == XMLHttpRequest.DONE) {
                var seqComponent = doc.responseText;
                var o = Qt.createQmlObject(seqComponent,root);
            }
        }
        doc.open("GET", "http://127.0.0.1:14445/TestComponent3.qml");
        doc.send();
    }
}
