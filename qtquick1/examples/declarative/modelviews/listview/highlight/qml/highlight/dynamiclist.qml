/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/
import QtQuick 1.0
import "content"

// This example shows how items can be dynamically added to and removed from
// a ListModel, and how these list modifications can be animated.

Rectangle {
    id: container
    width: 500; height: 400
    color: "#343434"

    // The model:
    ListModel {
        id: fruitModel

        ListElement {
            name: "Apple"; cost: 2.45
            attributes: [
                ListElement { description: "Core" },
                ListElement { description: "Deciduous" }
            ]
        }
        ListElement {
            name: "Banana"; cost: 1.95
            attributes: [
                ListElement { description: "Tropical" },
                ListElement { description: "Seedless" }
            ]
        }
        ListElement {
            name: "Cumquat"; cost: 3.25
            attributes: [
                ListElement { description: "Citrus" }
            ]
        }
        ListElement {
            name: "Durian"; cost: 9.95
            attributes: [
                ListElement { description: "Tropical" },
                ListElement { description: "Smelly" }
            ]
        }
    }

    // The delegate for each fruit in the model:
    Component {
        id: listDelegate

        Item {
            id: delegateItem
            width: listView.width; height: 55
            clip: true

            Row {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 10

                Column {
                    Image {
                        source: "content/pics/arrow-up.png"
                        MouseArea { anchors.fill: parent; onClicked: fruitModel.move(index, index-1, 1) }
                    }
                    Image { source: "content/pics/arrow-down.png"
                        MouseArea { anchors.fill: parent; onClicked: fruitModel.move(index, index+1, 1) }
                    }
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter

                    Text {
                        text: name
                        font.pixelSize: 15
                        color: "white"
                    }
                    Row {
                        spacing: 5
                        Repeater {
                            model: attributes
                            Text { text: description; color: "White" }
                        }
                    }
                }
            }

            Row {
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                spacing: 10

                PressAndHoldButton {
                    anchors.verticalCenter: parent.verticalCenter
                    source: "content/pics/plus-sign.png"
                    onClicked: fruitModel.setProperty(index, "cost", cost + 0.25)
                }

                Text {
                    id: costText
                    anchors.verticalCenter: parent.verticalCenter
                    text: '$' + Number(cost).toFixed(2)
                    font.pixelSize: 15
                    color: "white"
                    font.bold: true
                }

                PressAndHoldButton {
                    anchors.verticalCenter: parent.verticalCenter
                    source: "content/pics/minus-sign.png"
                    onClicked: fruitModel.setProperty(index, "cost", Math.max(0,cost-0.25))
                }

                Image {
                    source: "content/pics/list-delete.png"
                    MouseArea { anchors.fill:parent; onClicked: fruitModel.remove(index) }
                }
            }

            // Animate adding and removing of items:

            ListView.onAdd: SequentialAnimation {
                PropertyAction { target: delegateItem; property: "height"; value: 0 }
                NumberAnimation { target: delegateItem; property: "height"; to: 55; duration: 250; easing.type: Easing.InOutQuad }
            }

            ListView.onRemove: SequentialAnimation {
                PropertyAction { target: delegateItem; property: "ListView.delayRemove"; value: true }
                NumberAnimation { target: delegateItem; property: "height"; to: 0; duration: 250; easing.type: Easing.InOutQuad }

                // Make sure delayRemove is set back to false so that the item can be destroyed
                PropertyAction { target: delegateItem; property: "ListView.delayRemove"; value: false }
            }
        }
    }

    // The view:
    ListView {
        id: listView
        anchors.fill: parent; anchors.margins: 20
        model: fruitModel
        delegate: listDelegate
    }

    Row {
        anchors { left: parent.left; bottom: parent.bottom; margins: 20 }
        spacing: 10

        TextButton {
            text: "Add an item"
            onClicked: {
                fruitModel.append({
                    "name": "Pizza Margarita",
                    "cost": 5.95,
                    "attributes": [{"description": "Cheese"}, {"description": "Tomato"}]
                })
            }
        }

        TextButton {
            text: "Remove all items"
            onClicked: fruitModel.clear()
        }
    }
}

