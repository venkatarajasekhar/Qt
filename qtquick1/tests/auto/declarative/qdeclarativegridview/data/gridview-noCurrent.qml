import QtQuick 1.0

Rectangle {
    property int current: grid.currentIndex
    width: 240
    height: 320
    color: "#ffffff"
    resources: [
        Component {
            id: myDelegate
            Rectangle {
                id: wrapper
                objectName: "wrapper"
                width: 80
                height: 60
                border.color: "blue"
                Text {
                    text: index
                }
                Text {
                    x: 40
                    text: wrapper.x + ", " + wrapper.y
                }
                Text {
                    y: 20
                    id: textName
                    objectName: "textName"
                    text: name
                }
                Text {
                    y: 40
                    id: textNumber
                    objectName: "textNumber"
                    text: number
                }
                color: GridView.isCurrentItem ? "lightsteelblue" : "white"
            }
        }
    ]
    GridView {
        id: grid
        objectName: "grid"
        focus: true
        width: 240
        height: 320
        currentIndex: -1
        cellWidth: 80
        cellHeight: 60
        delegate: myDelegate
        model: testModel
    }
}
