import QtQuick 1.0

QtObject {
    property string url

    property bool dataOK: false
    property bool done: false

    Component.onCompleted: {
        var x = new XMLHttpRequest;
        x.open("GET", url);

        x.onreadystatechange = function() {
            if (x.readyState == XMLHttpRequest.DONE) {
                done = true;
                dataOK = x.status == 404;
            }
        }

        x.send();
    }
}

