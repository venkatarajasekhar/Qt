import QtQuick 1.0

Rectangle {
    width: 300; height: 300;
    Rectangle {
        objectName: "theRect"
        color: "red"
        width: 60; height: 60;
        x: 100; y: 100;
        SmoothedAnimation on x { objectName: "easeX"; to: 200; duration: 0 }
    }
}
