/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Graphical Effects module.
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

import QtQuick 2.0

Item {
    id: colorSlider

    property real value: 1
    property real maximum: 1
    property real minimum: 0
    property string caption: ""
    property bool pressed: mouseArea.pressed
    property bool integer: false
    property string handleSource: "images/slider_handle.png"
    property real handleOpacity: 1.0
    property alias trackItem: track.children

    width: parent.width
    height: 20

    function updatePos() {
        if (maximum > minimum) {
            var pos = (track.width - 10) * (value - minimum) / (maximum - minimum) + 5;
            return Math.min(Math.max(pos, 5), track.width - 5) - 10;
        } else {
            return 5;
        }
    }

    Text {
        id: captionText
        width: 110
        anchors.verticalCenter: parent.verticalCenter
        horizontalAlignment: Text.AlignRight
        text: colorSlider.caption + ':'
        font.family: "Arial"
        font.pixelSize: 11
        color: "#B3B3B3"
    }

    Text {
        id: valueCaption
        anchors.left: captionText.right
        anchors.leftMargin: 11
        anchors.verticalCenter: parent.verticalCenter
        width: 35
        horizontalAlignment: Text.AlignLeft
        text: colorSlider.value.toFixed(1)
        font.family: "Arial"
        font.pixelSize: 11
        color: "#999999"
    }

    Item {
        id: track
        height: 4
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: parent.width / 2
        anchors.right: parent.right
        anchors.rightMargin: 10

        Image {
            id: handle;
            anchors.verticalCenter: parent.verticalCenter
            smooth: true
            source: mouseArea.pressed ? 'images/slider_handle_pressed.png' : colorSlider.handleSource
            opacity: colorSlider.handleOpacity
            x: updatePos()
            z: 1
        }

        MouseArea {
            id: mouseArea
            anchors {left: parent.left; right: parent.right; leftMargin: 5; rightMargin: 5; verticalCenter: parent.verticalCenter}
            height: 8
            preventStealing: true

            onPressed: {
                var handleX = Math.max(0, Math.min(mouseX, mouseArea.width))
                var realValue = (maximum - minimum) * handleX / mouseArea.width + minimum;
                value = colorSlider.integer ? Math.round(realValue) : realValue;
            }

            onPositionChanged: {
                if (pressed) {
                    var handleX = Math.max(0, Math.min(mouseX, mouseArea.width))
                    var realValue = (maximum - minimum) * handleX / mouseArea.width + minimum;
                    value = colorSlider.integer ? Math.round(realValue) : realValue;
                }
            }
        }
    }
}
